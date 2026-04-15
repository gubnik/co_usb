#include "boost/capy/ex/run_async.hpp"
#include "boost/capy/ex/this_coro.hpp"
#include "boost/capy/task.hpp"
#include "co_usb/executor.hpp"
#include "co_usb/raii/device_handle.hpp"
#include "co_usb/transfer.hpp"
#include <boost/capy.hpp>
#include <co_usb/co_usb.hpp>
#include <libusb-1.0/libusb.h>
#include <print>

constexpr uint8_t total    = 8;
constexpr uint16_t dev_vid = 0x9f9f;
constexpr uint16_t dev_pid = 0x9f9f;
constexpr uint64_t tfersz  = 16 * 16 * 1024;

boost::capy::task<void> process_transfer (co_usb::device_handle::raw devh)
{
    uint8_t data[tfersz];
    auto tfer = libusb_alloc_transfer(0);
    libusb_fill_bulk_transfer(tfer, devh, 0x81, data, tfersz, nullptr, nullptr,
                              0);
    for (;;)
    {
        auto [ec, n] = co_await co_usb::transfer_awaitable(tfer, devh);
        std::println("{}", std::span{data, n});
    }
}

boost::capy::task<void> dispatch_transfers (co_usb::device_handle devh)
{
    const auto *env = co_await boost::capy::this_coro::environment;
    for (uint8_t i = 0; i < total; i++)
    {
        boost::capy::run_async(env->executor)(process_transfer(devh));
    }
}

int main (int argc, char **argv)
{
    co_usb::handler_loop hl{};
    co_usb::device_handle devh{
        libusb_open_device_with_vid_pid(hl.usb_context(), dev_vid, dev_pid)};
    if (!devh)
    {
        std::println(stderr, "Cannot open device!");
        return 1;
    }
    boost::capy::run_async(hl.get_executor())(
        dispatch_transfers(std::move(devh)));
    hl.run();
}
