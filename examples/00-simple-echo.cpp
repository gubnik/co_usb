/**
 * 00-simple-echo.cpp
 * Copyright (c) 2026 Nikolay Gubankov. Boost Software License 1.0.
 * Simple USB echo across 2 bulk endpoints (IN and OUT)
 *
 * Demonstrates compile-time direction semantic and basics of reads and writes.
 */

#include "co_usb/error.hpp"
#include <array>
#include <boost/capy.hpp>
#include <co_usb.hpp>
#include <print>

constexpr uint16_t dev_vid      = 0x9f9f;
constexpr uint16_t dev_pid      = 0x9f9f;
constexpr uint8_t dev_iface_num = 0;

boost::capy::task<> echo (const co_usb::interface &iface)
{
    std::array<uint8_t, 1024> buf;
    auto in_tfer  = co_usb::bulk_transfer{co_usb::ep_in(0x81, iface)};
    auto out_tfer = co_usb::bulk_transfer{co_usb::ep_out(0x02, iface)};
    for (;;)
    {
        auto [rec, rn] =
            co_await in_tfer.read_some(boost::capy::mutable_buffer{buf.data(), buf.size()});

        if (rec)
            break;

        auto [wec, wn] = co_await out_tfer.write_some(boost::capy::const_buffer{buf.data(), rn});

        if (wec)
            break;
    }
}

int main (int argc, char **argv)
{
    boost::capy::thread_pool tp;
    co_usb::context<> ctx(tp.get_executor());
    auto [dec, devh] = co_usb::open(ctx.get(), {dev_vid, dev_pid});
    if (dec)
    {
        std::println(stderr, "Error during device opening: {}", dec.message());
        return dec.value();
    }
    auto [gec, guard] = co_usb::kernel_driver_guard::detach(devh, dev_iface_num);
    if (gec && gec != co_usb::make_usb_error_code(co_usb::usb_error::not_found))
    {
        std::println(stderr, "Error during driver detachment: {}", gec.message());
        return gec.value();
    }
    auto [iec, iface] = co_usb::interface::claim(devh, dev_iface_num);
    if (iec)
    {
        std::println(stderr, "Error during interface claiming: {}", iec.message());
        return iec.value();
    }
    boost::capy::run_async(tp.get_executor())(echo(iface));
    tp.join();
}
