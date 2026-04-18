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

# Elements
- `co_usb::transfer_awaitable`, an `IoAwaitable` which submits the tranfer on suspension and resumes on arrival;
- `co_usb::context` wrapper around `libusb_context` with RAII and a built-in default handler thread *(totally optional to use!)*;
- `co_usb::unique_transfer`, `unique_dev_handle` and `interface_holder` RAII wrappers;
- `ReadStream`/`WriteStream` abstractions over endpoint reads (100% syntactic sugar);

# Wiki
Pending

# Example
See `/examples`
