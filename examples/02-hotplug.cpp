/**
 * 02-hotplug.cpp
 * Copyright (c) 2026 Nikolay Gubankov. Boost Software License 1.0.
 *
 * Minimal example demonstrating hotplug API usage for reconnecting devices
 *
 * Demonstrates high-level @ref co_usb::device_acceptor usage that matches
 * a familiar Asio-like asio::ip::tcp::acceptor interface.
 *
 * For raw @ref co_usb::hotplug_awaitable, see example 03
 */

#include <boost/capy.hpp>
#include <co_usb.hpp>
#include <libusb-1.0/libusb.h>
#include <print>

constexpr uint16_t dev_vid      = 0x9f9f;
constexpr uint16_t dev_pid      = 0x9f9f;
constexpr uint8_t dev_iface_num = 0;

boost::capy::task<> dev_loop (co_usb::unique_dev_handle devh)
{
    co_usb::interface iface{devh.get(), dev_iface_num};
    co_usb::bulk_transfer read_in{co_usb::ep_in(0x01, iface)};
    char buf[1024];
    for (;;)
    {
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
        auto [ec, dev] = co_await acceptor.accept(dev_vid, dev_pid, LIBUSB_HOTPLUG_MATCH_ANY);

        if (ec)
            break;

        std::println("Device arrived!");
        auto devh = co_usb::open(dev);
        boost::capy::run_async(exec)(dev_loop(std::move(devh)));
    }
}

int main (int argc, char **argv)
{
    boost::capy::thread_pool tp{1};
    co_usb::context<> ctx(tp.get_executor());
    boost::capy::run_async(tp.get_executor())(accept_hotplug(ctx.get()));
    tp.join();
}
