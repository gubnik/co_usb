#include "co_usb/hotplug/device_left_token.hpp"
#include "co_usb/hotplug/device_ref.hpp"
#include "co_usb/hotplug/hotplug_awaitable.hpp"
#include <boost/capy/io_task.hpp>
#include <co_usb/hotplug/device_acceptor.hpp>
#include <libusb-1.0/libusb.h>

co_usb::device_acceptor::device_acceptor (libusb_context *ctx) noexcept : m_ctx(ctx)
{
}

void co_usb::device_acceptor::set_enumeration (bool do_enumerate) noexcept
{
    m_do_enumerate = do_enumerate;
}

bool co_usb::device_acceptor ::do_enumerate () const noexcept
{
    return m_do_enumerate;
}

boost::capy::io_task<co_usb::device_ref> co_usb::device_acceptor::accept (int vid, int pid,
                                                                          int dev_class)
{
    auto [ec, res] =
        co_await co_usb::hotplug_awaitable{m_ctx,
                                           LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
                                           m_do_enumerate ? LIBUSB_HOTPLUG_ENUMERATE : 0,
                                           vid,
                                           pid,
                                           dev_class};
    co_return {ec, res.dev};
}

boost::capy::io_task<co_usb::device_ref, co_usb::device_left_signal>
co_usb::device_acceptor::accept_with_left (int vid, int pid, int dev_class)
{
    auto [ec, res] =
        co_await co_usb::hotplug_awaitable{m_ctx,
                                           LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
                                           m_do_enumerate ? LIBUSB_HOTPLUG_ENUMERATE : 0,
                                           vid,
                                           pid,
                                           dev_class};
    libusb_device_descriptor devdesc;
    libusb_get_device_descriptor(res.dev.get(), &devdesc);
    auto dlt = device_left_signal{m_ctx, devdesc.idVendor, devdesc.idProduct, devdesc.bDeviceClass};
    co_return {ec, res.dev, std::move(dlt)};
}
