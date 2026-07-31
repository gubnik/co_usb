#pragma once

#include <libusb.h>
#include <ranges>
#include <type_traits>

namespace co_usb::transfer::detail
{

template <typename Ty>
concept TransferResource =
    std::same_as<std::remove_cvref_t<Ty>, libusb_transfer *> || requires(Ty ty) {
        { ty.get() } -> std::same_as<libusb_transfer *>;
    };

template <TransferResource Ty>
constexpr inline auto transfer_of (Ty const &tfer_res) -> libusb_transfer *
{
    if constexpr (std::same_as<std::remove_cvref_t<Ty>, libusb_transfer *>)
    {
        return tfer_res;
    }
    else
    {
        return tfer_res.get();
    }
};

template <typename Ty>
concept TransferSequence =
    TransferResource<Ty> ||
    (std::ranges::bidirectional_range<Ty> && TransferResource<std::ranges::range_value_t<Ty>>);

} // namespace co_usb::transfer::detail
