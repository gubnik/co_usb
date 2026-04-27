#include "co_usb/hotplug/device_ref.hpp"
#include <co_usb/raii.hpp>
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

co_usb::unique_dev_handle co_usb::open (co_usb::device_ref dev)
{
    libusb_device_handle *devh;
    libusb_open(dev.raw(), &devh);
    return {devh, libusb_close};
}
