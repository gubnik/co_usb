#include "co_usb/error.hpp"
#include <co_usb/kernel_driver_guard.hpp>
#include <libusb-1.0/libusb.h>

co_usb::kernel_driver_guard::kernel_driver_guard (libusb_device_handle *devh, int iface_num)
    : m_devh{devh, [iface_num = iface_num] (libusb_device_handle *devh)
             { libusb_attach_kernel_driver(devh, iface_num); }},
      m_iface_num(iface_num)
{
}

boost::capy::io_result<co_usb::kernel_driver_guard>
co_usb::kernel_driver_guard::detach (libusb_device_handle *devh, int iface_num) noexcept
{
    auto guard           = kernel_driver_guard{devh, iface_num};
    auto r               = libusb_detach_kernel_driver(guard.m_devh.get(), guard.m_iface_num);
    std::error_code errc = make_usb_error_code(static_cast<usb_error>(r));
    return {errc, std::move(guard)};
}

boost::capy::io_result<co_usb::kernel_driver_guard>
co_usb::kernel_driver_guard::detach (unique_dev_handle &devh, int iface_num) noexcept
{
    auto guard           = kernel_driver_guard{devh.get(), iface_num};
    auto r               = libusb_detach_kernel_driver(guard.m_devh.get(), guard.m_iface_num);
    std::error_code errc = make_usb_error_code(static_cast<usb_error>(r));
    return {errc, std::move(guard)};
}

libusb_device_handle *co_usb::kernel_driver_guard::dev () const noexcept
{
    return m_devh.get();
}

int co_usb::kernel_driver_guard::number () const noexcept
{
    return m_iface_num;
}
