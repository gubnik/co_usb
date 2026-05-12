/**
 * 04-cancellation.cpp
 * Copyright (c) 2026 Nikolay Gubankov. Boost Software License 1.0.
 *
 * An example program demonstrating proper transfer cancellation logic via a relatively simple
 * "Hello, world!"-type program.
 *
 * It will likely not work for a random device :-)
 */

#include <atomic>
#include <boost/capy.hpp>
#include <co_usb.hpp>
#include <print>
#include <type_traits>

constexpr uint16_t dev_vid      = 0x9f9f;
constexpr uint16_t dev_pid      = 0x9f9f;
constexpr uint8_t dev_iface_num = 0;

// helper function to help with managing active work
// this is one of many ways to manage that and may be not ideal, it is not the part of co_usb's API
template <class F>
    requires std::invocable<F>
auto defer (F &&f)
{
    auto deleter    = [f = std::forward<F>(f)] (void *) { f(); };
    using deleter_t = std::decay_t<decltype(deleter)>;
    return std::unique_ptr<void, deleter_t>{(void *)1, deleter};
}

// Greets the device nicely
boost::capy::task<> dev_loop (co_usb::device_ref dev, std::atomic<size_t> &active_work)
{
    // increment active_work counter and decrement it at scope's end
    active_work.fetch_add(1, std::memory_order_release);
    auto defer_dec = defer([&active_work] { active_work.fetch_sub(1, std::memory_order_release); });

    // propagate the stop token
    auto stop_token = co_await boost::capy::this_coro::stop_token;

    // open the device
    auto [dec, devh] = co_usb::open(dev);
    if (dec)
    {
        std::println(stderr, "Error during device opening: {}", dec.message());
        co_return;
    }

    // guard to detach and reattach kernel driver
    auto [gec, guard] = co_usb::kernel_driver_guard::detach(devh, dev_iface_num);
    if (gec && gec != co_usb::make_usb_error_code(co_usb::usb_error::not_found))
    {
        std::println(stderr, "Error during driver detachment: {}", gec.message());
        co_return;
    }

    // claim the interface
    auto [iec, iface] = co_usb::interface::claim(devh, dev_iface_num);
    if (iec)
    {
        std::println(stderr, "Error during interface claiming: {}", iec.message());
        co_return;
    }

    // allocate and pre-fill the transfer
    // libusb doesn't have allocator API so we can't propagate frame allocator
    // this operation can only fail with bad_alloc, so no error code
    co_usb::bulk_transfer hello_tfer{co_usb::ep_out(0x02, iface)};

    // set up an stop observer to cancel outgoing transfers when the stop is requested
    // libusb_cancel_transfer is thread-safe so it is safe to call from this callback without
    // additional synchronization
    std::stop_callback cancel_tfer_cb{stop_token,
                                      [tfer = hello_tfer.raw()] { libusb_cancel_transfer(tfer); }};

    constexpr std::string_view hello = "Hello there, a friendly device!";
    while (!stop_token.stop_requested())
    {
        auto [wec, wn] =
            co_await hello_tfer.write_some(boost::capy::const_buffer{hello.data(), hello.size()});

        if (!wec)
        {
            continue;
        }

        auto err = static_cast<co_usb::transfer_status>(wec.value());

        // timeouts are normal, continue
        if (err == co_usb::transfer_status::timed_out)
        {
            continue;
        }

        if (err == co_usb::transfer_status::cancelled)
        {
            std::println("Transfer was cancelled; exiting the process loop...");
            co_return;
        }

        if (err == co_usb::transfer_status::no_device)
        {
            std::println(stderr, "Device was detached; exiting the process loop...");
            co_return;
        }

        // some other error we don't have anything special to say about
        std::println(stderr, "Critical transfer error: {}; exiting the process loop...",
                     wec.message());
        co_return;
    }
}

boost::capy::task<> accept_hotplug (libusb_context *ctx, std::atomic<size_t> &active_work)
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
        boost::capy::run_async(exec, stop_token, allocator)(dev_loop(dev, active_work));
    }
}

// the default handler does not provide any kind of tracking of pending events before
// exiting on stop_requested(), so we have to provide our own
auto make_tracking_event_handler (std::atomic<size_t> &active_work)
{
    return [&active_work] (libusb_context *ctx, std::stop_token st)
    {
        const timeval ctv = {.tv_sec = 0, .tv_usec = 10'000};
        for (;;)
        {
            timeval tv = ctv;
            auto r     = libusb_handle_events_timeout(ctx, &tv);
            if (r != LIBUSB_SUCCESS)
            {
                throw std::system_error{make_usb_error_code(static_cast<co_usb::usb_error>(r))};
            }
            // no work and cancellation was required - oblige and leave
            if (st.stop_requested() && active_work.load(std::memory_order_acquire) == 0)
            {
                break;
            }
        }
    };
}

int main (int argc, char **argv)
{
    boost::capy::thread_pool tp{1};

    // counter for the number of active libusb work to correctly time event loop cancellation
    // this is so transfer cancellations get handled and we don't end up in a broken state
    // some of the alternatives are: latches, semaphores, barries, etc.
    std::atomic<size_t> active_work{0};

    // initiates a libusb context and binds a libusb event handler thread to executor's execution
    // context with an associated stop source
    co_usb::context<> ctx(tp.get_executor(), make_tracking_event_handler(active_work));

    // start the acceptor loop with event handler stop token
    // this allows to gracefully exit with correct handling order
    boost::capy::run_async(tp.get_executor(),
                           ctx.get_token())(accept_hotplug(ctx.get(), active_work));
    tp.join();
}
