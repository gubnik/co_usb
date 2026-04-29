> [!IMPORTANT]
> This is an experimental project until this notice is removed

# co_usb

Modern C++ library using C++20 coroutines and [Boost.Capy](https://github.com/cppalliance/capy) to
create a lightweight interface for [libusb-1.0](https://libusb.info/). It provides minimal abstractions
over base libusb to enable efficient and clean concurrent I/O using Boost.Capy's common interfaces
so it can be used seamlessly with libraries that consume those interfaces, e.g. Boost.Http.

# Getting started

This project uses `vcpkg` as a package manager.
To use it in your `vcpkg`-based, projects:
1. Add [portfile](`./portfile.cmake`) and [vcpkg.json](./vcpkg.json) to your ports directory
2. Copy `./res/ports/boost-capy/` and `./res/ports/libusb/` to your ports dir
3. Provide them as overlay to the `vcpkg`
4. Add the following to your `CMakeLists.txt`:
```cmake
find_package(co_usb CONFIG REQUIRED)
target_link_libraries(your_project PRIVATE co_usb::co_usb)
```

For non-`vcpkg` projects, you'll have to use CMake's `FetchContent` module and provide Boost.Capy and libusb
on your own.

# Documentation

Pending. Refer to in-code comments until then.

# Design principles

`co_usb` is first and foremost designed to not get in the way of regular libusb code - every abstraction
lets the user of the library peel off the layers and get into raw libusb handles and functions if need be - it is no
goal of the library to be an all-encompassing wrapper over libusb.

However, `co_usb` does provide some high-level abstractions to provide a better interface - namely,
transfer types and hotplug API. They introduce a tiny overhead compared to raw awaitables and libusb but
it is negligible in the context of any I/O delay.

# Transfer awaitables (ReadStream/WriteStream)

Transfers are the primary asynchronous unit of libusb, and by extension `co_usb` as well. This library provides a
`transfer_awaitable` primitive for wrapping submission logic: submit a transfer upon suspension and resume when the transfer
callback was fired. This keeps the submission process quite straightforward:
```cpp
char buf[1024];
libusb_transfer *transfer = /* allocate and setup */;
transfer.buffer = buf;
auto [ec, n] = co_await co_usb::transfer_awaitable{transfer};
if (ec)
    /* handle error */;
process_data(buf, n);
```

`co_usb` also exposes *transfer types* as a higher lever API: these are wrappers over `transfer_awaitable` which
give correct-by-construction semantics to transfers and expose Capy's `ReadStream` and `WriteStream` API in a form of
`read_some` and `write_some` functions. As such, transfer types are fully qualified streams and can and should be passed
to other libraries expecting Capy's streams, such as Boost.Http or Boost.Corosio. Aside from semantics, they *do not incur any additional costs*,
they *do not* allocate or introduce *any additional storage*, at all:
```cpp
char buf[1024];
co_usb::bulk_transfer transfer{co_usb::ep_in(0x81, iface)};
auto [ec, n] = co_await transfer.read_some(boost::capy::mutable_buffer{buf, sizeof(buf)});
if (ec)
    /* handle error */;
process_data(buf, n);
```

# Hotplug API (Device acceptor)

Hotplug API is a major component of both libusb and `co_usb` - it is a way to get information about devices being attached and detached
in real time, and it is a rather complicated component of libusb in regards to managing device lifetimes.

`co_usb` aims to provide an awaitable interface to libusb's hotplug callbacks by introducing `hotplug_awaitable` type, which is exactly
what is reads like: an awaitable which, upon suspension, registers a callback and upon callback's completion resumes the awaiting coroutine
and deregisters the callback. It is a *very* low level wrapper over hotplug callbacks, and as such it does not allocate and should be used
for the cases which cannot be handled by higher level abstraction (see [example 03](./examples/03-raw-hotplug.cpp)). One overhead of
such awaitable is that it returns *not* the raw `libusb_device*` but a `co_usb::device_ref` which *guarantees* a non-null device
underneath, increments device ref count on construction and decrements on destruction.
```cpp
libusb_context *ctx = /* initialize */;
auto [ec, event, device] = co_await co_usb::hotplug_awaitable{
            ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
            LIBUSB_HOTPLUG_ENUMERATE, dev_vid, dev_pid, LIBUSB_HOTPLUG_MATCH_ANY};
if (ec)
    /* handle error */;
switch(event)
case co_usb::hotplug_event::arrived:
    /* handle arrival */;
case co_usb::hotplug_event::left:
    /* handle departure */;
```

`device_acceptor` is a higher level abstraction over hotplug callbacks, it closely mimics Asio's `asio::ip::tcp::acceptor`
and wraps `hotplug_awaitable` to provide familiar interface to hotplug interactions:
- `accept` method "accepts" a device via a simplified ARRIVED-only awaitable and gives the user a straightforward interface
for most of their needs (see [example 02](./examples/02-hotplug.cpp)).
```cpp
libusb_context *ctx = /* initialize */;
co_usb::device_acceptor acceptor(ctx);
auto [ec, device] = co_await acceptor.accept(dev_vid, dev_pid, LIBUSB_HOTPLUG_MATCH_ANY);
if (ec)
    /* handle error */;
/* open the device, submit transfers, etc. */
```
- `accept_with_left` method "accepts" the device in the same way as `accept` but also returns a `device_left_signal`, which is a
way to know when the device was detached, similar to `std::stop_token` for cancellation (see [example 04](./examples/04-left-hotplug.cpp)).
Please note that `device_left_signal` maintains a signal state similar to stop state of `std::stop_source` *and it is allocated on the heap*,
so each signal allocates once per lifetime on initial construction.
```cpp
libusb_context *ctx = /* initialize */;
co_usb::device_acceptor acceptor(ctx);
auto [ec, device, left_signal] = co_await acceptor.accept_with_left(dev_vid, dev_pid, LIBUSB_HOTPLUG_MATCH_ANY);
if (ec)
    /* handle error */;
for(;;)
{
    if (left_signal.device_left())
    {
        /* exit when device was detached */
        break;
    }
    /* open the device, submit transfers, etc. */
}
```

