#include "boost/capy/ex/run_async.hpp"
#include "boost/capy/ex/this_coro.hpp"
#include "co_usb/executor.hpp"
#include <co_usb/co_usb.hpp>
#include <boost/capy.hpp>
#include <libusb-1.0/libusb.h>
#include <print>

static uint8_t data[1024];

boost::capy::task<void> read_ctrl_in(libusb_device_handle *devh)
{
    auto env = co_await boost::capy::this_coro::environment;
    auto tfer = libusb_alloc_transfer(0);
    libusb_fill_bulk_transfer(tfer, devh, 0x81, data, 1024, nullptr, nullptr, 0);
    for (;;)
    {
        auto [ec, n] = co_await co_usb::transfer_awaitable(tfer, devh);
        std::println("{}", std::span{data, n});
    }
}

int main(int argc, char **argv)
{
    co_usb::handler_loop hl{};
    libusb_device_handle *devh = libusb_open_device_with_vid_pid(hl.usb_context(), 0x8087, 0x0aaa);
    if (!devh)
    {
        std::println(stderr, "Cannot open device!");
        return 1;
    }
    boost::capy::run_async(hl.get_executor())(read_ctrl_in(devh));
    hl.run();
}
