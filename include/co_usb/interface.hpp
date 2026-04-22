#pragma once

#include <libusb-1.0/libusb.h>
namespace co_usb
{

struct interface
{
    interface (libusb_device_handle *devh, int iface, bool detach_kernel_module = true)
        : m_devh(devh), m_iface(iface)
    {
        if (detach_kernel_module)
        {
            libusb_detach_kernel_driver(m_devh, m_iface);
        }
        libusb_claim_interface(m_devh, iface);
    }

    ~interface ()
    {
        libusb_release_interface(m_devh, m_iface);
    }

    auto dev () const noexcept
    {
        return m_devh;
    }

    auto number () const noexcept
    {
        return m_iface;
    }

  private:
    libusb_device_handle *m_devh;
    int m_iface;
};

} // namespace co_usb
