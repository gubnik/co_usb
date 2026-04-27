#include "co_usb/hotplug/device_ref.hpp"
#include "co_usb/hotplug/hotplug_awaitable.hpp"
#include <boost/capy/io_task.hpp>
#include <co_usb/hotplug/device_acceptor.hpp>
#include <libusb-1.0/libusb.h>

co_usb::device_acceptor::device_acceptor (libusb_context *ctx) noexcept : m_ctx(ctx)
{
}

boost::capy::task<co_usb::maybe_device_ref> co_usb::device_acceptor::accept (int vid, int pid,
                                                                             int dev_class)
{
    auto [ev, dev] = co_await co_usb::hotplug_awaitable{
        m_ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE, vid, pid, dev_class};
    co_return dev;
}

boost::capy::task<co_usb::maybe_device_ref> co_usb::accept (libusb_context *ctx, int vid, int pid,
                                                            int dev_class)
{
    auto [ev, dev] = co_await co_usb::hotplug_awaitable{
        ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE, vid, pid, dev_class};
    co_return dev;
}
