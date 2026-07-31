/**
 * @file 00-simple-echo.cpp
 * Copyright (c) 2026 Nikolay Gubankov. Boost Software License 1.0.
 * Simple USB echo across 2 bulk endpoints (IN and OUT)
 *
 * Demonstrates compile-time direction semantic and basics of reads and writes.
 */

#include <boost/capy.hpp>
#include <co_usb.hpp>

constexpr uint16_t dev_vid = 0x9f9f;
constexpr uint16_t dev_pid = 0x9f9f;
constexpr uint8_t dev_iface_num = 0;

// simple asynchronous operation that reads from one endpoint and writes to another
boost::capy::task<> echo (const co_usb::interface &iface)
{
    auto exec = co_await boost::capy::this_coro::executor;
    auto stop = co_await boost::capy::this_coro::stop_token;

    // create a buffer for transfers. Any kind of sized storage suffices for that since it'll be
    // wrapped into Capy's buffer types by pointer and size anyway. We use C-style array for
    // simplicity.
    char buf[1024];

    // co_usb transfer types such as bulk_transfer, or any other kind of transfer, are templated
    // on the endpoint direction. It can be deducted by providing a proper endpoint object, which
    // is simply a pair of endpoint number and a raw libusb device handle also templated on the
    // direction. This ensures that you cannot read from an out endpoint and write into the read
    // one. Of course, control_transfer supports both read and write operation, as per specification
    // for control endpoint, and does not take endpoint parameter at all.
    co_usb::transfer::resource tfers[2];
    co_usb::transfer::bulk_in in_tfer{exec, tfers[0], co_usb::transfer::ep_in(0x81, iface)};
    co_usb::transfer::bulk_out out_tfer{exec, tfers[1], co_usb::transfer::ep_out(0x02, iface)};
    while (!stop.stop_requested())
    {
        // partial read from an endpoint. Transfer types are qualified ReadStream/WriteStream and
        // can even be type-erased via boost::capy::any_read_stream/boost::capy::any_write_stream.
        // rn is the number of bytes actualy read from an endpoint and the actual data read is
        // stored in buf.
        auto [rec, rn] = co_await in_tfer.read_some(boost::capy::mutable_buffer{buf, sizeof(buf)});

        // for simplicity, we break on any error - this is semantically incorrect for any usage more
        // complicated than this example since it also includes timeouts as an error. But, since we
        // did not provide any timeout to the transfer we will not be getting this error.
        if (rec)
            break;

        // write the received data to the out endpoint. Notice that we use rn as the size instead of
        // buffer's full size since we echo only what we actually got.
        auto [wec, wn] = co_await out_tfer.write_some(boost::capy::const_buffer{buf, rn});

        if (wec)
            break;
    }
}

int main (int argc, char **argv)
{
    // co_usb is inherently Capy-based, which means we need an executor to run
    // operations on. We use thread pool for the examples but co_usb works with ANY
    // Capy-compliant executor, for example boost::corosio::io_context if you need
    // to interop with networking
    boost::capy::thread_pool tp;

    // creates and initiates libusb context with a default event handler service
    // bound to execution context that will last until the program finishes.
    // Event handler function can be provided via a functor as a second constructor parameter
    // if the default one is not enough - it often is not and should only be used for simple
    // examples such as this
    co_usb::ev::context ctx =
        co_usb::make_context<co_usb::ev::trivial_event_handler>(tp.get_executor());

    // co_usb consistently uses error_code + value schema for handling errors, especially
    // for thin wrappers over libusb functions. It provides error categories for both of libusb
    // error types, co_usb::usb_error enum and co_usb::usb_error_category for common libusb_error
    // which most functions return, and co_usb::transfer_status enum and
    // co_usb::transfer_status_category for specifically transfer status which is returned on
    // transfer operations including async ones

    // opens a device by a triplet.
    // a device triplet is a struct of 3 integers: vid, pid and dev_class, used for
    // all device-related operations instead of plain integer parameters. In case of
    // co_usb::open, the dev_class parameter is ignored and can be not provided.
    // It returns a unique_device_handle which acts as a unique_ptr - not to be confused with
    // device_ref.
    auto devh = co_usb::open_device(tp.get_executor(), {dev_vid, dev_pid}, co_usb::as_exception());
    // detaches a kernel driver from a device interface and creates a RAII guard that will reattach
    // it later. Acts like a shared pointer and can be freely copied
    auto maybe_guard = co_usb::detach_driver(devh, dev_iface_num, co_usb::as_expected());
    // claims the interface and returns interface object that will release it on lifetime end.
    // Interface object acts like a shared_ptr relative to its interface.
    auto iface = co_usb::claim_interface(devh, dev_iface_num, co_usb::as_exception());
    // starts an asynchronous operation on the executor
    boost::capy::run_async(tp.get_executor(), ctx.get_token())(echo(iface));
    tp.join();
}
