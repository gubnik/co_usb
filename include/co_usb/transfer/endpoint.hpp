#pragma once

#include "co_usb/interface.hpp"
#include <libusb-1.0/libusb.h>
#include <stdexcept>

namespace co_usb
{

enum class endpoint_type : uint8_t
{
    control = 0,
    bulk,
    interrupt,
    isochronous,
    bulk_stream,
};

enum class ep_direction
{
    out  = 0x00,
    in   = 0x80,
    both = 0xFF,
};

/**
 * @tparam Direction direction of an endpoint
 */
template <ep_direction Direction> struct endpoint
{
    /**
     * @brief makes an endpoint and completes address to proper value
     *
     * @details Example:
     * @li ep_direction::out ep = 0x83 -> 0x03
     * @li ep_direction::in ep = 0x83 -> 0x83
     * @li ep_direction::in ep = 0x03 -> 0x83
     */
    static endpoint<Direction> make_safe (uint8_t ep, const interface &iface) noexcept
    {
        if constexpr (Direction == ep_direction::out)
        {
            return {static_cast<uint8_t>(ep ^ LIBUSB_ENDPOINT_IN), iface.dev()};
        }
        else if constexpr (Direction == ep_direction::in)
        {
            return {static_cast<uint8_t>(ep | LIBUSB_ENDPOINT_IN), iface.dev()};
        }
        return {ep, iface.dev()};
    }

    static endpoint<Direction> make_unsafe (uint8_t ep, libusb_device_handle *devh) noexcept
    {
        return {ep, devh};
    }

    /**
     * @brief creates an endpoint or throws if the address doesn't match expected direction
     *
     * @throws @ref std::invalid_argument when endpoint address does not match @tp Direction
     */
    static endpoint<Direction> make_throwing (uint8_t ep, libusb_device_handle *devh)
    {
        if constexpr (Direction == ep_direction::out)
        {
            if (ep > LIBUSB_ENDPOINT_IN)
            {
                throw std::invalid_argument{"Cannot use IN endpoint for OUT"};
            }
        }
        else if constexpr (Direction == ep_direction::in)
        {
            if (ep < LIBUSB_ENDPOINT_IN)
            {
                throw std::invalid_argument{"Cannot use OUT endpoint for IN"};
            }
        }
        return {ep, devh};
    }

    uint8_t addr () const noexcept
    {
        return m_ep;
    }

    auto *dev () const noexcept
    {
        return m_devh;
    }

  private:
    endpoint (uint8_t ep, libusb_device_handle *devh) : m_ep(ep), m_devh(devh)
    {
    }

  private:
    uint8_t m_ep;
    libusb_device_handle *m_devh;
};

/**
 * @brief wrapper around endpoint<co_usb::ep_direction::in>::make_safe
 */
endpoint<ep_direction::in> ep_in(uint8_t ep, const interface &iface);

/**
 * @brief wrapper around endpoint<co_usb::ep_direction::out>::make_safe
 */
endpoint<ep_direction::out> ep_out(uint8_t ep, const interface &iface);

} // namespace co_usb
