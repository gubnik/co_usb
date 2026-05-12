#include <co_usb/transfer/endpoint.hpp>

co_usb::endpoint<co_usb::ep_direction::in> co_usb::ep_in (uint8_t ep,
                                                          const interface &iface) noexcept
{
    return endpoint<ep_direction::in>::make_safe(ep, iface);
}

co_usb::endpoint<co_usb::ep_direction::out> co_usb::ep_out (uint8_t ep,
                                                            const interface &iface) noexcept
{
    return endpoint<ep_direction::out>::make_safe(ep, iface);
}

co_usb::endpoint<co_usb::ep_direction::both> co_usb::ep_any (uint8_t ep,
                                                             const interface &iface) noexcept
{
    return endpoint<ep_direction::both>::make_unsafe(ep, iface.dev());
}
