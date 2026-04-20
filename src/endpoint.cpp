#include <co_usb/tfer/endpoint.hpp>

co_usb::endpoint<co_usb::ep_direction::in> co_usb::ep_in (uint8_t ep, libusb_device_handle *devh)
{
    return endpoint<ep_direction::in>::make_unsafe(ep, devh);
}

co_usb::endpoint<co_usb::ep_direction::out> co_usb::ep_out (uint8_t ep, libusb_device_handle *devh)
{
    return endpoint<ep_direction::out>::make_unsafe(ep, devh);
}
