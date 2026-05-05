/**
 * 04-left-hotplug.cpp
 * Copyright (c) 2026 Nikolay Gubankov. Boost Software License 1.0.
 *
 * Simple example demonstrating usage of @ref co_usb::left_signal for gracefully
 * managing device disconnection.
 */

#include "co_usb/hotplug/device_acceptor.hpp"
#include "co_usb/transfer/error.hpp"
#include <boost/capy.hpp>
#include <co_usb.hpp>
#include <libusb-1.0/libusb.h>
#include <print>

constexpr uint16_t dev_vid      = 0x9f9f;
constexpr uint16_t dev_pid      = 0x9f9f;
constexpr uint8_t dev_iface_num = 0;

// Intentionally primitive read loop
boost::capy::task<> dev_loop (co_usb::device_ref dev, co_usb::left_signal left_signal)
{
    auto devh = co_usb::open(dev);
    co_usb::interface iface{devh.get(), dev_iface_num};
    co_usb::bulk_transfer read_in{co_usb::ep_in(0x01, iface)};
    char buf[1024];
    for (;;)
    {

        auto [ec, n] = co_await read_in.read_some(boost::capy::mutable_buffer{buf, sizeof(buf)});

        // if device was detached - leave now
        if (left_signal.device_left())
        {
            std::println("Device was detached, stopping loop");
            break;
        }

        if (ec)
        {
            auto status = static_cast<co_usb::transfer_status>(ec.value());
            if (status == co_usb::transfer_status::timed_out)
                continue;
            std::println("Critical transfer error: {}", ec.message());
            // handle there
            break;
        }

        std::println("Got {} bytes", n);
    }
}

boost::capy::task<> accept_hotplug (libusb_context *ctx)
{
    auto exec = co_await boost::capy::this_coro::executor;

    // propagate the allocator to the acceptor
    // allows to provide a stack-based allocator to not use heap at all
    auto allocator = co_await boost::capy::this_coro::frame_allocator;

    // create the acceptor which tracks currently connected devices and accepts new ones
    // devices CAN safely outlive the acceptor and do not depend on it in any way!
    auto acceptor = co_usb::device_acceptor(ctx, allocator);
    for (;;)
    {
        std::println("Awaiting device...");
        // left_signal is a single-fire signal for a matching LEFT event on a device
        auto [ec, dev, left_signal] =
            co_await acceptor.accept(co_usb::use_left, {.vid = dev_vid, .pid = dev_pid});
        std::println("Received device!");

        if (ec)
            break;

        std::println("Device arrived!");
        boost::capy::run_async(exec)(dev_loop(dev, std::move(left_signal)));
    }
}

int main (int argc, char **argv)
{
    boost::capy::thread_pool tp{1};
    co_usb::context<> ctx(tp.get_executor());
    boost::capy::run_async(tp.get_executor())(accept_hotplug(ctx.get()));
    tp.join();
}
