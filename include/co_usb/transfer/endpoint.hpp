/**
 * @file endpoint.hpp
 * @brief USB endpoint aggregate templated on its direction.
 */

#pragma once

#include "co_usb/wrapper/detail/device_handle_source.hpp"
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace co_usb::transfer
{

/**
 * @ingroup transfer
 *
 * @brief Direction of an endpoint.
 */
enum class endpoint_direction
{
    out = 0x00, //< OUT endpoint (0x80 is not set)
    in = 0x80,  //< IN endpoint (0x80 is set)
    any = 0xFF, //< any/unknown direction
};

/**
 * @ingroup transfer
 *
 * @brief USB endpoint aggregate templated on its direction.
 *
 * @details Must be used with transfer types to provide directional information.
 * Prefer @ref make_safe function to construct a valid endpoint.
 *
 * @tparam Direction direction of an endpoint. Value co_usb::ep_direction::any is semantically
 * equal to an unknown endpoint direction and as such only allows casts to either in or out
 * endpoint.
 */
template <endpoint_direction Direction = endpoint_direction::any> struct endpoint
{
    /**
     * @brief Makes an endpoint and completes address to proper value
     *
     * @details Example:
     * @li ep_direction::out ep = 0x83 -> 0x03
     * @li ep_direction::in ep = 0x83 -> 0x83
     * @li ep_direction::in ep = 0x03 -> 0x83
     */
    template <co_usb::detail::DeviceHandleSource DevSourceTy>
    static endpoint<Direction> make_safe (uint8_t ep, DevSourceTy const &dev_source) noexcept
        requires(Direction != endpoint_direction::any)
    {
        if constexpr (Direction == endpoint_direction::out)
        {
            return endpoint<Direction>{static_cast<uint8_t>(ep & ~LIBUSB_ENDPOINT_IN),
                                       co_usb::detail::device_handle_of(dev_source)};
        }
        else if constexpr (Direction == endpoint_direction::in)
        {
            return endpoint<Direction>{static_cast<uint8_t>(ep | LIBUSB_ENDPOINT_IN),
                                       co_usb::detail::device_handle_of(dev_source)};
        }
        return endpoint<Direction>{ep, co_usb::detail::device_handle_of(dev_source)};
    }

    template <co_usb::detail::DeviceHandleSource DevSourceTy>
    static endpoint<Direction> make_unsafe (uint8_t ep, DevSourceTy const &devh) noexcept
    {
        return {ep, co_usb::detail::device_handle_of(devh)};
    }

    /**
     * @brief Creates an endpoint or throws if the address doesn't match expected direction
     *
     * @throws std::invalid_argument when endpoint address does not match Direction
     */
    template <co_usb::detail::DeviceHandleSource DevSourceTy>
    static endpoint<Direction> make_throwing (uint8_t ep, DevSourceTy const &devh)
        requires(Direction != endpoint_direction::any)
    {
        if constexpr (Direction == endpoint_direction::out)
        {
            if (ep & LIBUSB_ENDPOINT_IN)
            {
                throw std::invalid_argument{"Cannot use IN endpoint for OUT"};
            }
        }
        else if constexpr (Direction == endpoint_direction::in)
        {
            if (!(ep & LIBUSB_ENDPOINT_IN))
            {
                throw std::invalid_argument{"Cannot use OUT endpoint for IN"};
            }
        }
        return {ep, co_usb::detail::device_handle_of(devh)};
    }

    uint8_t addr () const noexcept
        requires(Direction != endpoint_direction::any)
    {
        return m_ep;
    }

    auto devh () const noexcept -> libusb_device_handle *
        requires(Direction != endpoint_direction::any)
    {
        return m_devh;
    }

    /**
     * @brief Cast an `any` endpoint to a concrete direction.
     *
     * @details Cast is performed on the basis of endpoint address containing LIBUSB_ENDPOINT_IN bit
     * set to 1.
     *
     * @returns std::nullopt if the cast cannot be performed.
     * @returns Optional value containing a properly typed endpoint if cast succeeded.
     */
    template <endpoint_direction ToDirection>
    std::optional<endpoint<ToDirection>> as () const noexcept
        requires(Direction == endpoint_direction::any && ToDirection != endpoint_direction::any)
    {
        if constexpr (ToDirection == endpoint_direction::in)
        {
            if (m_ep & LIBUSB_ENDPOINT_IN)
            {
                return endpoint<endpoint_direction::in>::make_unsafe(m_ep, m_devh);
            }
            return std::nullopt;
        }
        else if constexpr (ToDirection == endpoint_direction::out)
        {
            if (m_ep & LIBUSB_ENDPOINT_IN)
            {
                return std::nullopt;
            }
            return endpoint<endpoint_direction::out>::make_unsafe(m_ep, m_devh);
        }
    }

  private:
    endpoint (uint8_t ep, libusb_device_handle *devh) noexcept : m_devh(devh), m_ep(ep)
    {
    }

  private:
    libusb_device_handle *m_devh;
    uint8_t m_ep;
};

inline auto ep_out (uint8_t ep_addr, co_usb::detail::DeviceHandleSource auto const &devh_src)
    -> endpoint<endpoint_direction::out>
{
    return endpoint<endpoint_direction::out>::make_safe(ep_addr, devh_src);
}

inline auto ep_in (uint8_t ep_addr, co_usb::detail::DeviceHandleSource auto const &devh_src)
    -> endpoint<endpoint_direction::in>
{
    return endpoint<endpoint_direction::in>::make_safe(ep_addr, devh_src);
}

inline auto ep_any (uint8_t ep_addr, co_usb::detail::DeviceHandleSource auto const &devh_src)
    -> endpoint<endpoint_direction::any>
{
    return endpoint<endpoint_direction::any>::make_unsafe(
        ep_addr, co_usb::detail::device_handle_of(devh_src));
}

} // namespace co_usb::transfer
