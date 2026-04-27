#include "co_usb/hotplug/device_ref.hpp"
#include "co_usb/hotplug/hotplug_awaitable.hpp"
#include <boost/capy/io_task.hpp>
#include <co_usb/hotplug/device_acceptor.hpp>
#include <libusb-1.0/libusb.h>

co_usb::device_acceptor::device_acceptor (libusb_context *ctx) noexcept : m_ctx(ctx)
{
}

boost::capy::io_task<co_usb::device_ref> co_usb::device_acceptor::accept (int vid, int pid,
                                                                          int dev_class)
{
    auto [ec, res] = co_await co_usb::hotplug_awaitable{
        m_ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE, vid, pid, dev_class};
    co_return {ec, res.dev};
}

boost::capy::io_task<co_usb::device_ref> co_usb::accept (libusb_context *ctx, int vid, int pid,
                                                         int dev_class)
{
    auto [ec, res] = co_await co_usb::hotplug_awaitable{
        ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE, vid, pid, dev_class};
    co_return {ec, res.dev};
}
