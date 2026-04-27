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
    std::array<uint8_t, 16 * 16> data;
    // create a transfer (syntactic sugar over transfer_awaitable)
    co_usb::bulk_transfer tfer{co_usb::ep_in(0x81, iface), std::chrono::milliseconds{0'050}};
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
    // can be whatever, even Corosio's io_context!
    boost::capy::thread_pool tp{total};

    // create a context with a default event handler service
    static co_usb::context<> ctx(tp.get_executor());
    // or you can not depend on a default handler:
    // co_usb::context<co_usb::use_service::no> ctx();

    co_usb::unique_dev_handle devh = co_usb::open_vid_pid(ctx.get(), dev_vid, dev_pid);
    if (!devh)
    {
        std::println(stderr, "Cannot open device!");
        return 1;
    }

    // claim interface, detach driver and release on scope end (RAII)
    co_usb::interface iface{devh.get(), dev_iface_num};
    for (uint8_t i = 0; i < total; i++)
    {
        boost::capy::run_async(tp.get_executor(), ctx.get_token())(process_transfer(iface));
    }
    std::signal(SIGINT, [] (int) { ctx.request_stop(); });
    tp.join();
}
