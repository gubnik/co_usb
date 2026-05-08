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

boost::capy::task<> dev_loop (co_usb::device_ref dev)
{
    auto [dec, devh] = co_usb::open(dev);
    if (dec)
    {
        std::println(stderr, "Error during device opening: {}", dec.message());
        co_return;
    }
    auto [gec, guard] = co_usb::kernel_driver_guard::detach(devh, dev_iface_num);
    if (gec && gec != co_usb::make_usb_error_code(co_usb::usb_error::not_found))
    {
        std::println(stderr, "Error during driver detachment: {}", gec.message());
        co_return;
    }
    auto [iec, iface] = co_usb::interface::claim(devh, dev_iface_num);
    if (iec)
    {
        std::println(stderr, "Error during interface claiming: {}", iec.message());
        co_return;
    }
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
        auto [ec, dev] = co_await acceptor.accept({.vid = dev_vid, .pid = dev_pid});

        if (ec)
            break;

        std::println("Device arrived!");
        boost::capy::run_async(exec)(dev_loop(dev));
    }
}

int main (int argc, char **argv)
{
    boost::capy::thread_pool tp{1};
    co_usb::context<> ctx(tp.get_executor());
    boost::capy::run_async(tp.get_executor())(accept_hotplug(ctx.get()));
    tp.join();
}
