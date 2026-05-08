#include "co_usb/error.hpp"
#include <co_usb/interface.hpp>
#include <libusb-1.0/libusb.h>
#include <system_error>

co_usb::interface::interface (libusb_device_handle *devh, int iface) noexcept
    : m_devh(devh), m_iface_num(iface)
{
}

boost::capy::io_result<co_usb::interface> co_usb::interface::claim (libusb_device_handle *devh,
                                                                    int iface_num) noexcept
{
    auto r               = libusb_claim_interface(devh, iface_num);
    std::error_code errc = make_usb_error_code(usb_error::success);
    if (r != LIBUSB_SUCCESS)
    {
        errc = make_usb_error_code(static_cast<usb_error>(r));
    }
    return {errc, {devh, iface_num}};
}

boost::capy::io_result<co_usb::interface> co_usb::interface::claim (co_usb::unique_dev_handle &devh,
                                                                    int iface_num) noexcept
{
    return claim(devh.get(), iface_num);
}

void co_usb::interface::release () const noexcept
{
    if (m_devh)
        libusb_release_interface(m_devh, m_iface_num);
}

co_usb::interface::~interface ()
{
    release();
}

co_usb::interface::interface (interface &&other)
{
    release();
    m_devh       = other.m_devh;
    m_iface_num  = other.m_iface_num;
    other.m_devh = nullptr;
}

co_usb::interface &co_usb::interface::operator=(interface &&other)
{
    release();
    m_devh       = other.m_devh;
    m_iface_num  = other.m_iface_num;
    other.m_devh = nullptr;
    return *this;
}

libusb_device_handle *co_usb::interface::dev () const noexcept
{
    return m_devh;
}

int co_usb::interface::number () const noexcept
{
    return m_iface_num;
}
