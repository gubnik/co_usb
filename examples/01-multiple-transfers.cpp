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

#include "co_usb/wrapper/error_protocol.hpp"
#include <boost/capy.hpp>
#include <co_usb/co_usb.hpp>
#include <csignal>
#include <print>
#include <system_error>
#include <utility>

constexpr uint16_t dev_vid = 0x9f9f;
constexpr uint16_t dev_pid = 0x9f9f;
constexpr uint8_t dev_ep = 0x81;
constexpr uint8_t dev_iface_num = 0;

constexpr uint8_t total = 8;

boost::capy::task<void> process_transfer (const co_usb::interface &iface)
{
    using namespace std::chrono_literals;
    auto exec = co_await boost::capy::this_coro::executor;
    auto st = co_await boost::capy::this_coro::stop_token;

    std::array<uint8_t, 1024> data;
    co_usb::transfer_resource tfer_res;
    co_usb::bulk_transfer_read_stream tfer{exec, tfer_res, co_usb::endpoint_in(0x81, iface), 50ms};
    while (!st.stop_requested())
    {
        auto [ec, n] =
            co_await tfer.read_some(boost::capy::mutable_buffer{data.data(), data.size()});
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
    std::error_code ec;

    // create a context that references a newly create service with a specific event handler bound
    // to execution service this allows to not depend on a single type of executor and interop with
    // any Capy-based library
    static auto ctx =
        co_usb::make_context<co_usb::detail::refcounted_event_handler>(tp.get_executor());

    // throw std::system_error on error, we expect the device to exist and don't care for errors!
    auto devh = co_usb::open_device(tp.get_executor(), {dev_vid, dev_pid}, co_usb::as_exception());
    // error is irrelevant, we can check it but we are unbothered either way
    auto maybe_guard = co_usb::detach_driver(devh, dev_iface_num, co_usb::as_expected());
    // once again, just throw and hope
    auto iface = co_usb::claim_interface(devh, dev_iface_num, co_usb::as_exception());
    for (uint8_t i = 0; i < total; i++)
    {
        boost::capy::run_async(tp.get_executor(), ctx.get_token())(process_transfer(iface));
    }

    // rough cancellation example, do not do that in production
    std::signal(SIGINT, [] (int) { ctx.request_stop(); });
    tp.join();
}
