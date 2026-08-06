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
boost::capy::task<> echo (co_usb::interface const &iface)
{
    auto exec = co_await boost::capy::this_coro::executor;
    auto stop = co_await boost::capy::this_coro::stop_token;

    // create a buffer for transfers. Any kind of sized storage suffices for that since it'll be
    // wrapped into Capy's buffer types by pointer and size anyway. We use C-style array for
    // simplicity.
    char buf[1024];

    // allocate transfer resources. `co_usb` decouples transfer resource from transfer operation to
    // allow flexible usage.
    co_usb::transfer::resource tfers[2];

    // partial I/O operation transfer adapter. provides partial `read_some` operation.
    co_usb::transfer::bulk_in in_tfer{exec, tfers[0], co_usb::transfer::ep_in(0x81, iface)};

    // complete I/O operation transfer adapter. provides complete `write` operation.
    co_usb::transfer::bulk_sink out_tfer{exec, tfers[1], co_usb::transfer::ep_out(0x02, iface),
                                         sizeof(buf)}; // maximum size of a single transfer

    while (!stop.stop_requested())
    {
        // partial read operation. `rec` is an error code and `rn` is the number of bytes read,
        // which may be smaller than requested buffer size.
        auto [rec, rn] = co_await in_tfer.read_some(boost::capy::mutable_buffer{buf, sizeof(buf)});

        // for simplicity, we break on any error.
        // don't do that in real code.
        if (rec)
            break;

        // write all the data we received back to 0x02 endpoint.
        // unlike partial I/O `write_some`, complete `write` guarantees that all bytes will written.
        auto [wec, wn] = co_await out_tfer.write(boost::capy::const_buffer{buf, rn});

        if (wec)
            break;
    }
}

int main (int argc, char **argv)
{
    // co_usb is inherently Capy-based, which means we need an executor to run
    // operations on. We use thread pool for the examples but co_usb works with any
    // Capy-compliant executor, for example boost::corosio::io_context if you need
    // to interop with networking
    boost::capy::thread_pool tp;

    // creates the event handler service with a given handler.
    co_usb::ev::context ctx =
        co_usb::make_context<co_usb::ev::trivial_event_handler>(tp.get_executor());

    // for wrapper operations `co_usb` uses error protocol to wrap error code results.
    // this allows flexibility in the way errors and results are reported without forcing a single
    // opinion.
    // the default protocol for all wrapper functions is `as_exception`.

    // open a device from a triplet.
    auto devh = co_usb::open_device(tp.get_executor(), {dev_vid, dev_pid});

    // detach kernel driver from the interface.
    // we use `as_expected` to avoid error if the driver is absent.
    auto maybe_guard = co_usb::detach_driver(devh, dev_iface_num, co_usb::as_expected());

    // claim the interface.
    auto iface = co_usb::claim_interface(devh, dev_iface_num);

    boost::capy::run_async(tp.get_executor(), ctx.get_token())(echo(iface));
    tp.join();
}
