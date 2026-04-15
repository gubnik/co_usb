#pragma once

#include <libusb-1.0/libusb.h>

namespace co_usb
{

struct device_handle
{
    using raw = libusb_device_handle *;
    device_handle ()
    {
    }

    device_handle (libusb_device_handle *devh) : m_devh(devh)
    {
    }

    device_handle(const device_handle &)            = delete;
    device_handle &operator=(const device_handle &) = delete;

    device_handle (device_handle &&other)
    {
        release();
        m_devh = other.m_devh;
    }

    device_handle &operator=(device_handle &&other)
    {
        release();
        m_devh = other.m_devh;
        return *this;
    }

    ~device_handle ()
    {
        release();
    }

    void release ()
    {
        if (m_devh)
            libusb_close(m_devh);
        m_devh = nullptr;
    }

    auto get () noexcept
    {
        return m_devh;
    }

    const auto get () const noexcept
    {
        return m_devh;
    }

    operator bool () const noexcept
    {
        return (bool)m_devh;
    }

    operator libusb_device_handle *() const noexcept
    {
        return m_devh;
    }

  private:
    libusb_device_handle *m_devh = nullptr;
};

} // namespace co_usb
