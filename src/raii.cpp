#include "co_usb/error.hpp"
#include <co_usb/raii.hpp>
#include <libusb-1.0/libusb.h>
#include <system_error>

boost::capy::io_result<co_usb::unique_dev_handle> co_usb::open (libusb_context *ctx,
                                                                device_triplet triplet) noexcept
{
    auto devh            = libusb_open_device_with_vid_pid(ctx, triplet.vid, triplet.pid);
    std::error_code errc = make_usb_error_code(usb_error::success);
    if (!devh)
        errc = make_usb_error_code(usb_error::no_device);
    return {errc, {devh, libusb_close}};
}
boost::capy::io_result<co_usb::unique_dev_handle> co_usb::open (libusb_device *dev) noexcept
{
    libusb_device_handle *devh;
    auto r = libusb_open(dev, &devh);
    return {make_usb_error_code(static_cast<usb_error>(r)), {devh, libusb_close}};
}
boost::capy::io_result<co_usb::unique_dev_handle> co_usb::open (co_usb::device_ref dev) noexcept
{
    return open(dev.get());
}
