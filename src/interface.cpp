#include "co_usb/error.hpp"
#include <co_usb/interface.hpp>
#include <libusb-1.0/libusb.h>

co_usb::interface::interface (libusb_device_handle *devh, int iface_num) noexcept
    : m_devh(devh, [iface = iface_num] (libusb_device_handle *devh)
             { libusb_release_interface(devh, iface); }),
      m_iface_num(iface_num)
{
}

boost::capy::io_result<co_usb::interface> co_usb::interface::claim (libusb_device_handle *devh,
                                                                    int iface_num) noexcept
{
    auto r         = libusb_claim_interface(devh, iface_num);
    usb_error errc = usb_error::success;
    if (r != LIBUSB_SUCCESS)
    {
        errc = static_cast<usb_error>(r);
        devh = nullptr;
    }
    return {make_usb_error_code(errc), {devh, iface_num}};
}

boost::capy::io_result<co_usb::interface> co_usb::interface::claim (co_usb::unique_dev_handle &devh,
                                                                    int iface_num) noexcept
{
    return claim(devh.get(), iface_num);
}

void co_usb::interface::release () const noexcept
{
    if (m_devh)
        libusb_release_interface(m_devh.get(), m_iface_num);
}

libusb_device_handle *co_usb::interface::dev () const noexcept
{
    return m_devh.get();
}

int co_usb::interface::number () const noexcept
{
    return m_iface_num;
}
