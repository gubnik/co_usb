#include <co_usb/tfer/endpoint.hpp>

co_usb::endpoint<co_usb::ep_direction::in> co_usb::ep_in (uint8_t ep, const interface &iface)
{
    return endpoint<ep_direction::in>::make_safe(ep, iface);
}

co_usb::endpoint<co_usb::ep_direction::out> co_usb::ep_out (uint8_t ep, const interface &iface)
{
    return endpoint<ep_direction::out>::make_safe(ep, iface);
}
