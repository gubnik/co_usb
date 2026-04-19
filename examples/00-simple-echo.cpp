/**
 * Simple USB echo across 2 bulk endpoints (IN and OUT)
 *
 * Demonstrates compile-time direction semantic and basics of reads and writes.
 */

#include "co_usb/raii.hpp"
#include <array>
#include <boost/capy.hpp>
#include <co_usb.hpp>

constexpr uint16_t dev_vid  = 0x9f9f;
constexpr uint16_t dev_pid  = 0x9f9f;
constexpr uint8_t dev_iface = 0;

boost::capy::task<> echo (libusb_device_handle *devh)
{
    std::array<uint8_t, 1024> buf;
    auto in_tfer  = co_usb::bulk_transfer{co_usb::ep_in(0x81, devh)};
    auto out_tfer = co_usb::bulk_transfer{co_usb::ep_out(0x02, devh)};
    auto [ec, n]  = co_await in_tfer.read_some(boost::capy::mutable_buffer{buf.data(), buf.size()});
    (void)co_await out_tfer.write_some(boost::capy::const_buffer{buf.data(), n});
}

int main (int argc, char **argv)
{
    boost::capy::thread_pool tp;
    co_usb::context<> ctx(tp.get_executor());
    co_usb::unique_dev_handle devh = co_usb::open_vid_pid(ctx.get(), dev_vid, dev_pid);
    if (!devh)
        return 1;
    co_usb::interface_holder iface{devh.get(), dev_iface};
    boost::capy::run_async(tp.get_executor())(echo(devh.get()));
    tp.get_executor();
}
