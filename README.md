> [!IMPORTANT]
> This is an experimental project until this notice is removed

# co_usb

Modern C++ library using C++20 coroutines and [Boost.Capy](https://github.com/cppalliance/capy) to
create a lightweight interface for [libusb-1.0](https://libusb.info/).

# Design philosophy

The library is designed to *not get in your way* of writing regular `libusb` or `Boost.Capy` code.
Its interface consists of primitive awaitables satisfying [IoAwaitable](https://develop.capy.cpp.al/capy/4.coroutines/4d.io-awaitable.html)
protocol and minimal RAII wrappers around `libusb` primitives (context, device handle etc.).

# Wiki
Pending

# Example usage

> [!CAUTION]
> This may not be functioning code *just yet*!
```cpp
#include <boost/capy.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <co_usb/co_usb.hpp>
#include <print>

constexpr uint8_t total    = 8;
constexpr uint16_t dev_vid = 0x9f9f;
constexpr uint16_t dev_pid = 0x9f9f;
constexpr uint64_t tfersz  = 16 * 16 * 1024;

boost::capy::task<void> process_transfer (libusb_device_handle *devh)
{
    uint8_t data[tfersz];
    auto tfer = co_usb::raii::unique_transfer{libusb_alloc_transfer(0), libusb_free_transfer};
    libusb_fill_bulk_transfer(tfer.get(), devh, 0x81, data, tfersz, nullptr, nullptr, 0);
    for (;;)
    {
        auto [ec, n] = co_await co_usb::transfer_awaitable(tfer.get());
        if (ec)
        {
            std::println("Got error: {}", ec.message());
            continue;
        }
        std::println("Got data: {}", std::string_view{(char *)data, n});
    }
}

boost::capy::task<void> dispatch_transfers (co_usb::raii::unique_dev_handle devh)
{
    const auto *env = co_await boost::capy::this_coro::environment;
    for (uint8_t i = 0; i < total; i++)
    {
        boost::capy::run_async(env->executor, env->stop_token)(process_transfer(devh.get()));
    }
}

int main (int argc, char **argv)
{
    co_usb::context ctx;
    boost::capy::thread_pool tp{8};
    co_usb::raii::unique_dev_handle devh{
        libusb_open_device_with_vid_pid(ctx.get(), dev_vid, dev_pid), libusb_close};
    if (!devh)
    {
        std::println(stderr, "Cannot open device!");
        return 1;
    }
    boost::capy::run_async(tp.get_executor(), ctx.get_token())(ctx.spawn_handler());
    boost::capy::run_async(tp.get_executor(), ctx.get_token())(dispatch_transfers(std::move(devh)));
    tp.join();
}
```
