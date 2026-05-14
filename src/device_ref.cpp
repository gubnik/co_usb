#include <co_usb/device_ref.hpp>

co_usb::device_ref::device_ref () noexcept : m_dev(nullptr)
{
}

co_usb::device_ref::device_ref (libusb_device *dev) noexcept : m_dev(dev)
{
    libusb_ref_device(m_dev);
}

co_usb::device_ref::~device_ref () noexcept
{
    if (m_dev)
        libusb_unref_device(m_dev);
}

libusb_device *co_usb::device_ref::get () const noexcept
{
    return m_dev;
}

co_usb::device_ref::device_ref (const device_ref &other) noexcept
{
    m_dev = other.m_dev;
    if (m_dev)
        libusb_ref_device(m_dev);
}

co_usb::device_ref &co_usb::device_ref::operator=(const device_ref &other) noexcept
{
    m_dev = other.m_dev;
    if (m_dev)
        libusb_ref_device(m_dev);
    return *this;
}
