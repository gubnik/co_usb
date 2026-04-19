> [!IMPORTANT]
> This is an experimental project until this notice is removed

# co_usb

Modern C++ library using C++20 coroutines and [Boost.Capy](https://github.com/cppalliance/capy) to
create a lightweight interface for [libusb-1.0](https://libusb.info/). It provides minimal abstractions
over base `libusb` to enable efficient and clean concurrent I/O using `Boost.Capy`'s common interfaces
so it can be used seamlessly with libraries that consume those interfaces, e.g. `Boost.Http`.

# Design philosophy

The library is designed to *not get in your way* of writing regular `libusb` or `Boost.Capy` code.
Its interface consists of primitive awaitables satisfying [IoAwaitable](https://develop.capy.cpp.al/capy/4.coroutines/4d.io-awaitable.html)
protocol and minimal RAII wrappers around `libusb` primitives (context, device handle etc.).

The library builds on multiple levels of abstractions that allow wide range of possible usages:
1. Raw `transfer_awaitable` that accepts any `libusb_transfer*`, it should be used for maximum control over transfers
2. Wrapped `xxx_transfer` types which act as `ReadStream` or `WriteStream` - 100% syntactic sugar for compatibility with `Capy`-based libraries

# Wiki

Pending

# Example

See `/examples`
