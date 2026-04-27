#include <co_usb/hotplug/device_ref.hpp>
#include <libusb-1.0/libusb.h>
#include <stdexcept>

co_usb::device_ref::device_ref (libusb_device *dev) : m_dev(dev)
{
    if (!m_dev)
        throw std::runtime_error{"Cannot construct a null device_ref!"};
    libusb_ref_device(m_dev);
}

co_usb::device_ref::~device_ref () noexcept
{
    if (m_dev)
        libusb_unref_device(m_dev);
}

libusb_device *co_usb::device_ref::raw () const noexcept
{
    return m_dev;
}

co_usb::device_ref::device_ref (const device_ref &other) noexcept
{
    m_dev = other.m_dev;
    libusb_ref_device(m_dev);
}

co_usb::device_ref &co_usb::device_ref::operator=(const device_ref &other) noexcept
{
    m_dev = other.m_dev;
    libusb_ref_device(m_dev);
    return *this;
}
