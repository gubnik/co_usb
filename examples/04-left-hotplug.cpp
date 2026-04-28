/**
 * 04-left-hotplug.cpp
 * Copyright (c) 2026 Nikolay Gubankov. Boost Software License 1.0.
 *
 * Example demonstrating usage of hotplug API @ref co_usb::device_left_signal
 */

#include <boost/capy.hpp>
#include <co_usb.hpp>
#include <libusb-1.0/libusb.h>
#include <print>

constexpr uint16_t dev_vid      = 0x9f9f;
constexpr uint16_t dev_pid      = 0x9f9f;
constexpr uint8_t dev_iface_num = 0;

boost::capy::task<> dev_loop (co_usb::unique_dev_handle devh,
                              co_usb::device_left_signal left_signal)
{
    co_usb::interface iface{devh.get(), dev_iface_num};
    co_usb::bulk_transfer read_in{co_usb::ep_in(0x01, iface)};
    char buf[1024];
    for (;;)
    {
        if (left_signal.device_left())
        {
            std::println("Device was detached, stopping loop");
            break;
        }
        auto [ec, n] = co_await read_in.read_some(boost::capy::mutable_buffer{buf, sizeof(buf)});

        if (ec)
            break;

        std::println("Got {} bytes", n);
    }
}

boost::capy::task<> accept_hotplug (libusb_context *ctx)
{
    auto exec     = co_await boost::capy::this_coro::executor;
    auto acceptor = co_usb::device_acceptor(ctx);
    for (;;)
    {
        /*
         * devls stands for @ref device_left_signal
         *
         * It is a signaling mechanism for handling matching LEFT callbacks
         */
        auto [ec, dev, left_signal] =
            co_await acceptor.accept_with_left(dev_vid, dev_pid, LIBUSB_HOTPLUG_MATCH_ANY);

        if (ec)
            break;

        std::println("Device arrived!");
        auto devh = co_usb::open(dev);
        boost::capy::run_async(exec)(dev_loop(std::move(devh), std::move(left_signal)));
    }
}

int main (int argc, char **argv)
{
    boost::capy::thread_pool tp{1};
    co_usb::context<> ctx(tp.get_executor());
    boost::capy::run_async(tp.get_executor())(accept_hotplug(ctx.get()));
    tp.join();
}
