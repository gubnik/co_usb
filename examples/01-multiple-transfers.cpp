/**
 * 01-multiple-transfers.cpp
 * Copyright (c) 2026 Nikolay Gubankov. Boost Software License 1.0.
 *
 * Example of multiple asynchronous transfers running on a thread pool
 *
 * Demonstrates cancellation semantics and classic read loop.
 *
 * WARNING: there is a real chance your device or libusb backend is not built
 * for this kind of parallel transfers and will err or break spontaneously.
 * I test all my examples on a primitive virtual device in QEMU.
 */

#include <boost/capy.hpp>
#include <co_usb.hpp>
#include <csignal>
#include <print>

constexpr uint8_t total         = 8;
constexpr uint16_t dev_vid      = 0x9f9f;
constexpr uint16_t dev_pid      = 0x9f9f;
constexpr uint8_t dev_ep        = 0x81;
constexpr uint8_t dev_iface_num = 0;

boost::capy::task<void> process_transfer (const co_usb::interface &iface)
{
    auto st = co_await boost::capy::this_coro::stop_token;

    std::array<uint8_t, 1024> data;
    co_usb::bulk_transfer tfer{
        co_usb::ep_in(0x81, iface), std::chrono::milliseconds{0'050} // timeout
    };
    while (!st.stop_requested())
    {
        auto [ec, n] = co_await tfer.read_some({data.data(), data.size()});
        if (ec)
        {
            std::println("Got error: {}", ec.message());
            continue;
        }
        std::println("Got data: {}", std::string_view{(char *)data.data(), n});
    }
    std::println("Gracefully exited");
}

int main (int argc, char **argv)
{
    boost::capy::thread_pool tp{total};

    // create a context with a default event handler service bound to execution service
    // this allows to not depend on a single type of executor and interop with any Capy library
    static co_usb::context<> ctx(tp.get_executor());

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

    for (uint8_t i = 0; i < total; i++)
    {
        boost::capy::run_async(tp.get_executor(), ctx.get_token())(process_transfer(iface));
    }
    // rough cancellation example, do not do that in production
    std::signal(SIGINT, [] (int) { ctx.request_stop(); });
    tp.join();
}
