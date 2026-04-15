#include "boost/capy/ex/run_async.hpp"
#include "boost/capy/ex/this_coro.hpp"
#include "co_usb/executor.hpp"
#include <co_usb/co_usb.hpp>
#include <boost/capy.hpp>
#include <libusb-1.0/libusb.h>
#include <print>

static char data[1024];

boost::capy::task<void> read_ctrl_in(libusb_device_handle *devh)
{
    auto env = co_await boost::capy::this_coro::environment;
    auto tfer = libusb_alloc_transfer(0);
    
    for (;;)
    {
        auto [ec, n] = co_await co_usb::transfer_awaitable(tfer, devh, boost::capy::mutable_buffer{data, 1024});
        std::println("{}", std::span{data, 1024});
    }
}

int main(int argc, char **argv)
{
    co_usb::handler_loop hl{};
    libusb_device_handle *devh = libusb_open_device_with_vid_pid(hl.usb_context(), 0x8087, 0x0aaa);
    if (!devh)
    {
        std::println(stderr, "Niggerlicious device!");
        return 0;
    }
    boost::capy::run_async(hl.get_executor())(read_ctrl_in(devh));
    hl.run();
}
