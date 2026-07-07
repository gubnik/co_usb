#pragma once

#include <concepts>
#include <libusb.h>
#include <type_traits>
#include <utility>

namespace co_usb::detail
{

template <typename Ty>
concept DeviceHandlePointer =
    std::same_as<std::remove_cvref_t<Ty>, libusb_device_handle *> || requires(const Ty dev) {
        { dev.get() } -> std::same_as<libusb_device_handle *>;
    };

template <typename Ty>
concept DeviceHandleSource = DeviceHandlePointer<Ty> || requires(const Ty src) {
    { src.devh() } -> DeviceHandlePointer;
};

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

template <DeviceHandleSource Ty> auto device_handle_of (Ty &&devh_src) -> libusb_device_handle *
{
    if constexpr (DeviceHandlePointer<Ty>)
    {
        return device_handle_of(std::forward<Ty>(devh_src));
    }
    return device_handle_of(std::forward<Ty>(devh_src).devh());
}

} // namespace co_usb::detail
