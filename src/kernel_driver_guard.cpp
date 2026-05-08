#include "co_usb/error.hpp"
#include <co_usb/kernel_driver_guard.hpp>
#include <libusb-1.0/libusb.h>

co_usb::kernel_driver_guard::kernel_driver_guard (libusb_device_handle *devh, int iface_num)
    : m_devh{devh, [iface_num = iface_num] (libusb_device_handle *devh)
             { libusb_attach_kernel_driver(devh, iface_num); }},
      m_iface_num(iface_num)
{
    auto r = libusb_detach_kernel_driver(m_devh.get(), m_iface_num);
    if (r != LIBUSB_SUCCESS)
    {
        throw std::system_error{make_usb_error_code(static_cast<usb_error>(r))};
    }
}

co_usb::kernel_driver_guard::kernel_driver_guard (std::error_code &errc, libusb_device_handle *devh,
                                                  int iface_num) noexcept
    : m_devh(devh), m_iface_num(iface_num)
{
    auto r = libusb_detach_kernel_driver(m_devh.get(), m_iface_num);
    if (r != LIBUSB_SUCCESS)
    {
        errc = make_usb_error_code(static_cast<usb_error>(r));
    }
}

libusb_device_handle *co_usb::kernel_driver_guard::dev () const noexcept
{
    return m_devh.get();
}

int co_usb::kernel_driver_guard::number () const noexcept
{
    return m_iface_num;
}
