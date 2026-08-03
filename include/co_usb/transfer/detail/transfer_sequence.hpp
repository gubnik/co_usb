/**
 * @file transfer_sequence.hpp
 * @brief Transfer resource and transfer sequence concepts.
 */

#pragma once

#include <libusb.h>
#include <ranges>
#include <type_traits>

namespace co_usb::transfer::detail
{

/**
 * @ingroup transfer
 *
 * @concept TransferResource
 * @tparam Ty Type satisfying the TransferResource contract.
 *
 * @brief A type which can provide a `libusb_transfer *`.
 *
 * @details A single transfer resource object which may be queried to obtain a `libusb_transfer *`.
 * Must either be a `libusb_transfer *` itself or provide `.get() -> libusb_transfer *` method.
 */
template <typename Ty>
concept TransferResource =
    std::same_as<std::remove_cvref_t<Ty>, libusb_transfer *> || requires(Ty ty) {
        { ty.get() } -> std::same_as<libusb_transfer *>;
    };

/**
 * @ingroup transfer
 *
 * @brief Uniform accessor for obtaining a `libusb_transfer *` from
 * a @ref co_usb::transfer::detail::TransferResource.
 *
 * @returns `libusb_transfer *`
 */
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

/**
 * @ingroup transfer
 *
 * @concept TransferSequence
 * @tparam Ty Type satisfying the TransferSequence contract.
 *
 * @brief A sequence of @ref co_usb::transfer::detail::TransferResource.
 *
 * @details A sequence may consist either of a range of transfer resources or of a single
 * transfer resource, similar to Capy's buffer sequences.
 *
 * @see co_usb::transfer::detail::TransferResource
 */
template <typename Ty>
concept TransferSequence =
    TransferResource<Ty> ||
    (std::ranges::bidirectional_range<Ty> && TransferResource<std::ranges::range_value_t<Ty>>);

} // namespace co_usb::transfer::detail
