/**
 * @file 04-cancellation.cpp
 * Copyright (c) 2026 Nikolay Gubankov. Boost Software License 1.0.
 *
 * An example program demonstrating proper transfer cancellation logic via a relatively simple
 * "Hello, world!"-type program.
 *
 * It will likely not work for a random device :-)
 */

#include "co_usb/ev/context.hpp"
#include "co_usb/ev/detail/event_handler.hpp"
#include "co_usb/hotplug/device_acceptor.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/wrapper/device_handle.hpp"
#include "co_usb/wrapper/error_protocol.hpp"
#include <array>
#include <boost/capy.hpp>
#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <chrono>
#include <co_usb/co_usb.hpp>
#include <csignal>
#include <libusb.h>
#include <print>
#include <thread>

constexpr size_t working_seconds = 3;

constexpr uint16_t dev_vid      = 0x9f9f;
constexpr uint16_t dev_pid      = 0x9f9f;
constexpr uint8_t dev_iface_num = 0;

// Greets the device nicely
boost::capy::task<> dev_loop (co_usb::device_ref dev = {})
{
    auto exec = co_await boost::capy::this_coro::executor;
    auto stop = co_await boost::capy::this_coro::stop_token;

    std::error_code ec;
    // open the device
    auto maybe_devh = dev.valid() ? co_usb::open_device(dev, co_usb::as_optional(ec))
                                  : co_usb::open_device(exec,
                                                        {
                                                            .vid = dev_vid,
                                                            .pid = dev_pid,
                                                        },
                                                        co_usb::as_optional(ec));
    if (ec)
    {
        std::println(stderr, "Error during device opening: {}", ec.message());
        co_return;
    }
    auto devh = std::move(*maybe_devh);

    // guard to detach and reattach kernel driver
    auto maybe_guard = co_usb::detach_driver(devh, dev_iface_num, co_usb::as_expected());

    // claim the interface
    auto maybe_iface = co_usb::claim_interface(devh, dev_iface_num, co_usb::as_optional(ec));
    if (ec)
    {
        std::println(stderr, "Error during interface claiming: {}", ec.message());
        co_return;
    }
    auto iface = std::move(*maybe_iface);

    // allocate and pre-fill the transfer
    // libusb doesn't have allocator API so we can't propagate frame allocator
    using namespace std::chrono_literals;
    co_usb::bulk_transfer<co_usb::endpoint_direction::out, 4> hello_tfer{
        exec, co_usb::endpoint_out(0x02, iface), 50ms};
    co_usb::bulk_transfer<co_usb::endpoint_direction::in, 4> in_tfer{
        exec, co_usb::endpoint_in(0x01, iface), 50ms};

    size_t total_bytes{0};

    constexpr std::string_view hello   = "Hello there, a friendly device!";
    constexpr std::string_view goodbye = "Goodbye, a friendly device!";
    std::array bufs{
        boost::capy::const_buffer{hello.data(), hello.size()},
        boost::capy::const_buffer{goodbye.data(), goodbye.size()},
        boost::capy::const_buffer{hello.data(), hello.size()},
        boost::capy::const_buffer{goodbye.data(), goodbye.size()},
        boost::capy::const_buffer{hello.data(), hello.size()},
        boost::capy::const_buffer{goodbye.data(), goodbye.size()},
        boost::capy::const_buffer{hello.data(), hello.size()},
        boost::capy::const_buffer{goodbye.data(), goodbye.size()},
        boost::capy::const_buffer{hello.data(), hello.size()},
        boost::capy::const_buffer{goodbye.data(), goodbye.size()},
        boost::capy::const_buffer{hello.data(), hello.size()},
        boost::capy::const_buffer{goodbye.data(), goodbye.size()},
        boost::capy::const_buffer{hello.data(), hello.size()},
        boost::capy::const_buffer{goodbye.data(), goodbye.size()},
    };
    constexpr size_t bufsz = 1 << 18;
    char buf[16][bufsz];
    std::array<boost::capy::mutable_buffer, 16> bufs_in;
    for (size_t i = 0; i < bufs_in.size(); i++)
    {
        bufs_in[i] = boost::capy::mutable_buffer{buf[i], bufsz};
    }
    while (!stop.stop_requested())
    {
        // auto [wec, wn] = co_await hello_tfer.write_some(boost::capy::make_buffer(hello));
        // total_bytes += wn;
        auto [rec, rn] = co_await in_tfer.read_some(bufs_in);
        total_bytes += rn;
        if (!rec)
        {
            std::println("Got {} bytes", rn);
        }
        else
        {
            std::println(stderr, "Critical transfer error: {}; exiting the process loop...",
                         rec.message());
            break;
        }

        // std::println("Wrote {} bytes", wn);
        if (!rec)
        {
            continue;
        }

        // some other error we don't have anything special to say about
        std::println(stderr, "Critical transfer error: {}; exiting the process loop...",
                     rec.message());
        break;
    }
    std::println("Coro exit, total bytes transfered: {}", total_bytes);
    auto gbit = total_bytes * 8 * 1e-9;
    std::println("Average throughput: {} Gbit/second", gbit / working_seconds);
}

boost::capy::task<> accept_hotplug ()
{
    auto exec  = co_await boost::capy::this_coro::executor;
    auto stop  = co_await boost::capy::this_coro::stop_token;
    auto alloc = co_await boost::capy::this_coro::frame_allocator;
    std::error_code ec;
    co_usb::device_acceptor acceptor{exec, alloc};
    ec = acceptor.bind({.vid = 0x9f9f, .pid = 0x9f9f});
    ec = acceptor.listen();
    if (ec)
    {
        std::println(stderr, "Failed to bind hotplug router with error: `{}` (code={})",
                     ec.message(), ec.value());
    }

    while (!stop.stop_requested())
    {
        auto [ec, dev] = co_await acceptor.accept();

        if (ec)
        {
            std::println("Exiting router loop with error: `{}` (code={})", ec.message(),
                         ec.value());
            break;
        }

        // start the device processing loop with propagated executor, stop token and allocator
        boost::capy::run_async(exec, stop, alloc)(dev_loop(dev));
    }
    acceptor.shutdown();
}

volatile sig_atomic_t g_sigint = 0;

int main (int argc, char **argv)
{
    boost::capy::thread_pool tp{2};

    // initiates a libusb context and binds a libusb event handler thread to executor's execution
    // context with an associated stop source
    co_usb::context ctx =
        co_usb::make_context<co_usb::detail::refcounted_event_handler>(tp.get_executor());

    boost::capy::run_async(
        tp.get_executor(), [] () {}, [] (std::exception_ptr e) {})(
        [] (co_usb::context &ctx) -> boost::capy::task<>
        {
            // !! HACK ALERT
            // It is not recommended to do timed cancellation this way. Capy provides async_waker
            // you are ought to use with a dedicated timer thread to do such patter. However, as to
            // not bloat the example I will allow myself such dirty hack.
            std::this_thread::sleep_for(std::chrono::seconds{working_seconds});

            // old remove API
            // auto [ec] = co_await boost::capy::delay(std::chrono::seconds{1});

            ctx.request_stop();
            co_return;
        }(ctx));

    // start the acceptor loop with event handler stop token
    // this allows to gracefully exit with correct handling order
    boost::capy::run_async(tp.get_executor(), ctx.get_token())(accept_hotplug());
    tp.join();
}
