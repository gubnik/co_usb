#pragma once

#include <libusb-1.0/libusb.h>
namespace co_usb
{
enum class detach_kernel_module
{
    no = 0,
    yes
};

template <detach_kernel_module DetachModule = detach_kernel_module::yes> struct interface
{
    interface (libusb_device_handle *devh, int iface) : m_devh(devh), m_iface(iface)
    {
        if constexpr (DetachModule == detach_kernel_module::yes)
        {
            libusb_detach_kernel_driver(m_devh, m_iface);
        }
        libusb_claim_interface(m_devh, iface);
    }

    ~interface ()
    {
        libusb_release_interface(m_devh, m_iface);
    }

    auto device_handle () const noexcept
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
