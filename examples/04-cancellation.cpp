/**
 * 04-cancellation.cpp
 * Copyright (c) 2026 Nikolay Gubankov. Boost Software License 1.0.
 *
 * An example demonstrating proper transfer cancellation logic via a simple "Hello, world!"-type
 * program.
 *
 * It will likely not work for a random device :-)
 */

#include <boost/capy.hpp>
#include <co_usb.hpp>
#include <print>

constexpr uint16_t dev_vid      = 0x9f9f;
constexpr uint16_t dev_pid      = 0x9f9f;
constexpr uint8_t dev_iface_num = 0;

// Greets the device nicely
boost::capy::task<> dev_loop (co_usb::device_ref dev)
{
    // propagate the stop token
    auto stop_token = co_await boost::capy::this_coro::stop_token;

    auto devh = co_usb::open(dev);

    // RAII guard to detach and reattach kernel driver
    auto guard = co_usb::kernel_driver_guard{devh.get(), dev_iface_num};

    // claim the interface
    auto iface = co_usb::interface{devh.get(), dev_iface_num};

    // allocate and pre-fill the transfer
    // libusb doesn't have allocator API so we can't propagate frame allocator
    co_usb::bulk_transfer hello_tfer{co_usb::ep_out(0x02, iface)};

    // set up an stop observer to cancel outgoing transfers when the stop is requested
    // libusb_cancel_transfer is thread-safe so it is safe to call from this callback without
    // additional synchronization
    std::stop_callback cancel_tfer_cb{stop_token,
                                      [tfer = hello_tfer.raw()] { libusb_cancel_transfer(tfer); }};

    constexpr std::string_view hello = "Hello there, a friendly device!";
    while (!stop_token.stop_requested())
    {
        auto [ec, n] =
            co_await hello_tfer.write_some(boost::capy::const_buffer{hello.data(), hello.size()});

        if (!ec)
        {
            continue;
        }

        switch (static_cast<co_usb::transfer_status>(ec.value()))
        {

        case co_usb::transfer_status::completed:
        case co_usb::transfer_status::timed_out: // completion and timeout are not considered
                                                 // errors for resubmition loop
            continue;
        case co_usb::transfer_status::cancelled:
        {
            std::println("Transfer was cancelled; exiting the process loop...");
            co_return;
        }
        case co_usb::transfer_status::no_device:
        {
            std::println(stderr, "Device was detached; exiting the process loop...");
            co_return;
        }
        case co_usb::transfer_status::stall: // treat stall as error for simplicity
        case co_usb::transfer_status::error:
        case co_usb::transfer_status::overflow:
        {
            std::println(stderr, "Critical transfer error: {}; exiting the process loop...",
                         ec.message());
            co_return;
        };
        }
    }
}

boost::capy::task<> accept_hotplug (libusb_context *ctx)
{
    auto exec       = co_await boost::capy::this_coro::executor;
    auto stop_token = co_await boost::capy::this_coro::stop_token;
    auto allocator  = co_await boost::capy::this_coro::frame_allocator;

    co_usb::device_acceptor acceptor{ctx, allocator};
    while (!stop_token.stop_requested())
    {
        auto [ec, dev] = co_await acceptor.accept({.vid = dev_vid, .pid = dev_pid});

        if (ec) // should never happen unless we really messed up!
        {
            break;
        }

        // start the device processing loop with propagated executor, stop token and allocator
        boost::capy::run_async(exec, stop_token, allocator)(dev_loop(dev));
    }
}

int main (int argc, char **argv)
{
    boost::capy::thread_pool tp{1};

    // initiates a libusb context and binds a libusb event handler thread to executor's execution
    // context with an associated stop source
    co_usb::context<> ctx(tp.get_executor());

    // start the acceptor loop with event handler stop token
    // the default handler only exits after stop was requested and no events are left to handle
    // this allows to gracefully exit with correct handling order
    boost::capy::run_async(tp.get_executor(), ctx.get_token())(accept_hotplug(ctx.get()));
    tp.join();
}
