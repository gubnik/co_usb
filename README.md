> [!IMPORTANT]
> This is an experimental project until this notice is removed

# co_usb

Modern C++ library using C++20 coroutines and [Boost.Capy](https://github.com/cppalliance/capy) to
create a lightweight interface for [libusb-1.0](https://libusb.info/).

# Design philosophy

The library is designed to *not get in your way* of writing regular `libusb` or `Boost.Capy` code.
Its interface consists of primitive awaitables satisfying [IoAwaitable](https://develop.capy.cpp.al/capy/4.coroutines/4d.io-awaitable.html)
protocol and minimal RAII wrappers around `libusb` primitives (context, device handle etc.).

# Goal usage

> [!CAUTION]
> This is not functioning code *just yet*!

```cpp
static char data[1024];

boost::capy::task<void> read_ctrl_in(libusb_context *ctx)
{
    auto tfer = libusb_alloc_transfer(0);
    
    co_usb::device_handle devh{};
    for (;;)
    {
        auto [st, dev] = co_await co_usb::hotplug_event(ctx);
        if (st == co_usb::hotplug::left)
        {
            devh.reset();
            continue;
        }
        else if (st == co_usb::hotplug::arrived)
        {
            auto ret = libusb_open(ctx, &devh.get());
            if (ret != LIBUSB_SUCCESS)
            {
                std::println(stderr, "Cannot open device!");
                continue;
            }
        }
        auto ep = co_usb::ep{co_usb::bulk, devh, 0x81};
        auto [ec, n] = co_await ep.read_some(tfer, boost::capy::mutable_buffer{data, 1024});
        std::println("{}", std::span{data, 1024});
    }
}

int main(int argc, char **argv)
{
    co_usb::handler_loop hl{};
    boost::capy::run_async(hl.get_executor())(read_ctrl_in(hl.usb_context()));
    hl.run();
}
```
