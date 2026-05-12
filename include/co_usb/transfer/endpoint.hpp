#pragma once

#include "co_usb/interface.hpp"
#include <cstdint>
#include <libusb-1.0/libusb.h>
#include <optional>
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
 * @brief USB endpoint templated on its direction
 *
 * @details Must be used with transfer types to provide directional information.
 * Prefer @ref make_safe function to construct a valid endpoint
 *
 * @tparam Direction direction of an endpoint. Value co_usb::ep_direction::both is semantically
 * equal to an unknown endpoint direction and as such only allows casts to either in or out
 * endpoint.
 */
template <ep_direction Direction> struct endpoint
{
    /**
     * @brief Makes an endpoint and completes address to proper value
     *
     * @details Example:
     * @li ep_direction::out ep = 0x83 -> 0x03
     * @li ep_direction::in ep = 0x83 -> 0x83
     * @li ep_direction::in ep = 0x03 -> 0x83
     */
    static endpoint<Direction> make_safe (uint8_t ep, const interface &iface) noexcept
        requires(Direction != ep_direction::both)
    {
        if constexpr (Direction == ep_direction::out)
        {
            return {static_cast<uint8_t>(ep & ~LIBUSB_ENDPOINT_IN), iface.dev()};
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
     * @brief Creates an endpoint or throws if the address doesn't match expected direction
     *
     * @throws @ref std::invalid_argument when endpoint address does not match @tp Direction
     */
    static endpoint<Direction> make_throwing (uint8_t ep, libusb_device_handle *devh)
        requires(Direction != ep_direction::both)
    {
        if constexpr (Direction == ep_direction::out)
        {
            if (ep & LIBUSB_ENDPOINT_IN)
            {
                throw std::invalid_argument{"Cannot use IN endpoint for OUT"};
            }
        }
        else if constexpr (Direction == ep_direction::in)
        {
            if (!(ep & LIBUSB_ENDPOINT_IN))
            {
                throw std::invalid_argument{"Cannot use OUT endpoint for IN"};
            }
        }
        return {ep, devh};
    }

    uint8_t addr () const noexcept
        requires(Direction != ep_direction::both)
    {
        return m_ep;
    }

    auto *dev () const noexcept
        requires(Direction != ep_direction::both)
    {
        return m_devh;
    }

    /**
     * @brief Safe cast from an endpoint with an unknown direction to an endpoint
     * with a concrete direction
     *
     * @returns std::nullopt when the endpoint value does not match expected direction or is equal
     * to control endpoint (0x80)
     * @returns Properly typed endpoint if conversion is possible
     */
    template <ep_direction ToDirection>
    std::optional<endpoint<ToDirection>> as () const noexcept
        requires(Direction == ep_direction::both)
    {
        if constexpr (ToDirection == ep_direction::in)
        {
            if (m_ep & LIBUSB_ENDPOINT_IN)
            {
                return endpoint<ep_direction::in>::make_unsafe(m_ep, m_devh);
            }
            return std::nullopt;
        }
        else if constexpr (ToDirection == ep_direction::out)
        {
            if (m_ep & LIBUSB_ENDPOINT_IN)
            {
                return std::nullopt;
            }
            return endpoint<ep_direction::out>::make_unsafe(m_ep, m_devh);
        }
    }

  private:
    template <ep_direction ToDirection>
    friend endpoint<ToDirection> endpoint_cast(endpoint<ep_direction::both> ep) noexcept;

    endpoint (uint8_t ep, libusb_device_handle *devh) noexcept : m_ep(ep), m_devh(devh)
    {
    }

  private:
    uint8_t m_ep;
    libusb_device_handle *m_devh;
};

/**
 * @brief Wrapper around endpoint<co_usb::ep_direction::in>::make_safe
 */
endpoint<ep_direction::in> ep_in(uint8_t ep, const interface &iface) noexcept;

/**
 * @brief Wrapper around endpoint<co_usb::ep_direction::out>::make_safe
 */
endpoint<ep_direction::out> ep_out(uint8_t ep, const interface &iface) noexcept;

/**
 * @brief Creates an endpoint with an unknown direction
 */
endpoint<ep_direction::both> ep_any(uint8_t ep, const interface &iface) noexcept;

} // namespace co_usb
