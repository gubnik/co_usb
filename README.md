> [!IMPORTANT]
> Experimental - may be unstable, expect all kind of changes

# co_usb

Modern C++ library using C++20 coroutines and [Boost.Capy](https://github.com/cppalliance/capy) to
create a lightweight interface for [libusb-1.0](https://libusb.info/). It provides minimal abstractions
over base libusb to enable efficient and clean concurrent I/O using Boost.Capy's common interfaces
so it can be used seamlessly with libraries that consume those interfaces, e.g. Boost.Http.

# Getting started

This project uses `vcpkg` as a package manager.
To use it in your `vcpkg`-based projects, do the following:
1. Add [portfile](`./portfile.cmake`) and [vcpkg.json](./vcpkg.json) to your ports directory
2. Copy `./res/ports/boost-capy/` and `./res/ports/libusb/` to your ports dir
3. Provide them as overlay to the `vcpkg`
4. Add the following to your `CMakeLists.txt`:
```cmake
find_package(co_usb CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE co_usb::co_usb)
```

For non-`vcpkg` projects, you will have to use CMake's `FetchContent` module and provide
Boost.Capy and libusb on your own. This is method of consuming `co_usb` is not endorsed and may not work.
```cmake
include(FetchContent)
FetchContent_Declare(co_usb
    GIT_REPOSITORY https://github.com/gubnik/co_usb.git
    GIT_TAG dev
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(co_usb)

target_link_libraries(my_app co_usb::co_usb)
```

# Documentation

Pending. Refer to in-code comments until then.

# Design principles

`co_usb` is designed to be a helping layer on top of libusb and not an all-encompassing wrapper. It follows
original libusb abstractions closely and exposes original handles on every of its abstractions.

Still, the library does provide higher level mechanisms of interacting with libusb which are modeled after
established I/O libraries, namely - Asio and Corosio. These higher level abstraction do introduce a tiny overhead
compared to raw awaitables but in real code this overhead is negligible compared to I/O delays.

# Transfer awaitables and transfer types

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

# Hotplug awaitables and device acceptor

Hotplug API is a major component of both libusb and `co_usb` - it is a way to get information about devices being attached and detached
in real time, and it is a rather complicated component of libusb in regards to managing devices' lifetimes.

`co_usb` aims to provide an awaitable interface to libusb's hotplug callbacks by introducing `hotplug_awaitable` type.
This is an awaitable which, upon suspension, registers a callback, resumes the awaiting coroutine once the callback completes
and deregisters the callback. It is a low level wrapper over hotplug callbacks, and as such it does not allocate and should be used
for the cases which cannot be handled by a higher level abstraction (see [example 03](./examples/03-raw-hotplug.cpp)).
The overhead of this awaitable is that it returns *not* the raw `libusb_device*` but a `co_usb::device_ref` instead
which *guarantees* a non-null device and maintains a positivie ref count for the device during its lifetime.
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
way to know when the device was detached (see [example 04](./examples/04-left-hotplug.cpp)).
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
- - Similar to `std::stop_source`, `device_left_signal` maintains a signal state.
Each signal object allocates a tiny state on the heap on construction.
This is required because the state address must remain stable across moves and must outlive
both the acceptor and the coroutine inside which it was initially created.

# Handler service

By default, `co_usb` provides a *service* on a separate thread based on Boost.Capy's execution context
and bound to `co_usb::context<>` which performs the simplest possible cancellable libusb event handling.
The context constructor requires a valid Boost.Capy executor to be provided. It accepts any proper Boost.Capy executor,
ranging from `boost::capy::thread_pool` to `boost::corosio::io_context` to even custom executors.

> [!WARNING]
> Default handler is NOT thread-safe; it will break if attempted to be used on multiple threads

Handler function can be configured by providing additional argument to context constructor, which is a function pointer of
signature `void(libusb_context*, std::stop_token)`, or disabled entirely by providing `co_usb::use_service::no` as a template
parameter to `co_usb::context`. Note that if the service was disabled the context *will not* provide any kind of handler *nor*
a cancellation mechanism - you will have to implement them on your own.

```cpp
boost::capy::thread_pool pool{8};
co_usb::context<> ctx(pool.get_executor());              // default template is with service
co_usb::context<> ctx(pool.get_executor(), &my_handler); // with custom handler
co_usb::context<co_usb::use_service::no> ctx();          // disables the service and internal cancellation
```

## Cancellation

Context with enabled service provides cancellation semantics via exposed `std::stop_token`. It is recommended to be passed to
`boost::capy::run_async` if proper cancellation is needed. Due to the nature of libusb event handling the cancellation
will not be instantaneous and will happen *after* the current event is processed.

