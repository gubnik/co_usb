#pragma once

#include <libusb-1.0/libusb.h>
namespace co_usb
{

struct interface
{
    interface (libusb_device_handle *devh, int iface, bool detach_kernel_module = true)
        : m_devh(devh), m_iface(iface), m_module_detached(detach_kernel_module)
    {
        if (m_module_detached)
        {
            libusb_detach_kernel_driver(m_devh, m_iface);
        }
        libusb_claim_interface(m_devh, iface);
    }

    ~interface ()
    {
        if (m_module_detached)
        {
            libusb_attach_kernel_driver(m_devh, m_iface);
        }
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
    bool m_module_detached = true;
};

} // namespace co_usb
