#pragma once

#include <concepts>
#include <libusb.h>
#include <type_traits>
#include <utility>

namespace co_usb::detail
{

/**
 * @ingroup wrapper
 *
 * @concept DeviceHandlePointer
 * @tparam Ty Type satisfying the DeviceHandlePointer contract.
 *
 * @brief A type which can provide `libusb_device_handle *`.
 *
 * @details A single object which may be queried to obtain a `libusb_device_handle *`.
 * Must either be a `libusb_device_handle *` itself or provide `.get() -> libusb_device_handle *`
 * method.
 */
template <typename Ty>
concept DeviceHandlePointer =
    std::same_as<std::remove_cvref_t<Ty>, libusb_device_handle *> || requires(const Ty dev) {
        { dev.get() } -> std::same_as<libusb_device_handle *>;
    };

/**
 * @ingroup wrapper
 *
 * @concept DeviceHandleSource
 * @tparam Ty Type satisfying the DeviceHandleSource contract.
 *
 * @brief A type which can provide a @ref co_usb::detail::DeviceHandlePointer.
 *
 * @details A single object which may be queried to obtain a `DeviceHandlePointer auto`.
 * Must either be a `DeviceHandlePointer` itself or provide `.get() -> DeviceHandlePointer auto`
 * method.
 */
template <typename Ty>
concept DeviceHandleSource = DeviceHandlePointer<Ty> || requires(const Ty src) {
    { src.devh() } -> DeviceHandlePointer;
};

/**
 * @ingroup wrapper
 *
 * @brief Obtains a `libusb_device_handle *` from a @ref co_usb::detail::DeviceHandlePointer.
 *
 * @tparam Ty type which models DeviceHandlePointer
 * @param devh instance of type Ty
 *
 * @details Depending on the exact nature of the type will either return the object itself or
 * call `get`.
 */
template <DeviceHandlePointer Ty> auto device_handle_of (Ty &&devh) -> libusb_device_handle *
{
    if constexpr (std::same_as<std::remove_cvref_t<Ty>, libusb_device_handle *>)
    {
        return devh;
    }
    else
    {
        return std::forward<Ty>(devh).get();
    }
}

/**
 * @ingroup wrapper
 *
 * @brief Obtains a `libusb_device_handle *` from a @ref co_usb::detail::DeviceHandleSource.
 *
 * @tparam Ty type which models DeviceHandleSource
 * @param devh instance of type Ty
 *
 * @details Depending on the exact nature of the type will either return the result of
 * `device_handle_of` call on the object itself or on the result of `get`.
 */
template <DeviceHandleSource Ty> auto device_handle_of (Ty &&devh_src) -> libusb_device_handle *
{
    if constexpr (DeviceHandlePointer<Ty>)
    {
        return device_handle_of(std::forward<Ty>(devh_src));
    }
    return device_handle_of(std::forward<Ty>(devh_src).devh());
}

} // namespace co_usb::detail
