#include <co_usb/raii.hpp>
#include <format>
#include <libusb-1.0/libusb.h>

co_usb::unique_dev_handle co_usb::open_vid_pid (libusb_context *ctx, uint16_t vid, uint16_t pid)
{
    auto devh = libusb_open_device_with_vid_pid(ctx, vid, pid);
    return {devh, libusb_close};
}

co_usb::unique_dev_handle co_usb::open (libusb_device *dev)
{
    libusb_device_handle *devh;
    libusb_open(dev, &devh);
    return {devh, libusb_close};
}

co_usb::interface_holder::interface_holder (libusb_device_handle *devh, int interface_num)
    : m_devh(devh), m_interface_num(interface_num)
{
    auto r = libusb_claim_interface(m_devh, m_interface_num);
    if (r != LIBUSB_SUCCESS)
    {
        throw std::runtime_error{std::format("Could not claim the interface {}", m_interface_num)};
    }
}
co_usb::interface_holder::~interface_holder () noexcept
{
    libusb_release_interface(m_devh, m_interface_num);
}
