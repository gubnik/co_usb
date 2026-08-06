/**
 * @file operations.hpp
 * @brief Operations on transfer sequences.
 */

#pragma once

#include "co_usb/detail/assert.hpp"
#include "co_usb/transfer/detail/iterator.hpp"
#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include <chrono>
#include <libusb.h>

namespace co_usb::transfer
{

/**
 * @ingroup transfer
 *
 * @brief Cancels transfers from a sequence.
 *
 * @tparam TSeq Transfer sequence type.
 *
 * @details Calls `libusb_cancel_transfer` on all transfers obtained from transfer resources
 * from a given sequence. It is safe to call while the transfers are in flight.
 */
template <detail::TransferSequence TSeq> constexpr auto cancel_transfer (TSeq const &tfer_seq)
{
    auto it = detail::transfer_begin(tfer_seq);
    for (; it != detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_cancel_transfer(tfer);
    }
}

/**
 * @ingroup transfer
 *
 * @brief Prefills transfer sequence as control transfers.
 * @tparam TSeq Transfer sequence type.
 *
 * @note Control transfers are only valid for endpoints 0x00 (OUT) and 0x80 (IN).
 */
template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
constexpr auto prefill_control (TSeq const &tfer_seq, endpoint<EpDirection> ep,
                                std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
{

    COUSB_ASSERT((ep.addr() == 0x00) && (EpDirection == endpoint_direction::out) &&
                 "OUT control transfers must have an endpoint address of 0x00");
    COUSB_ASSERT((ep.addr() == 0x80) && (EpDirection == endpoint_direction::in) &&
                 "IN control transfers must have an endpoint address of 0x80");
    auto it = detail::transfer_begin(tfer_seq);
    for (; it != detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_fill_control_transfer(tfer, ep.devh(), nullptr, nullptr, nullptr,
                                     timeout_ms.count());
    }
}

/**
 * @ingroup transfer
 *
 * @brief Prefills transfer sequence as bulk transfers.
 * @tparam TSeq Transfer sequence type.
 */
template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
constexpr auto prefill_bulk (TSeq const &tfer_seq, endpoint<EpDirection> ep,
                             std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
{
    auto it = detail::transfer_begin(tfer_seq);
    for (; it != detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_fill_bulk_transfer(tfer, ep.devh(), ep.addr(), nullptr, 0, nullptr, nullptr,
                                  timeout_ms.count());
    }
}

/**
 * @ingroup transfer
 *
 * @brief Prefills transfer sequence as interrupt transfers.
 * @tparam TSeq Transfer sequence type.
 */
template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
constexpr auto
prefill_interrupt_transfer (TSeq const &tfer_seq, endpoint<EpDirection> ep,
                            std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
{
    auto it = detail::transfer_begin(tfer_seq);
    for (; it != detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_fill_interrupt_transfer(tfer, ep.devh(), ep.addr(), nullptr, 0, nullptr, nullptr,
                                       timeout_ms.count());
    }
}

/**
 * @ingroup transfer
 *
 * @brief Prefills transfer sequence as isochronous transfers.
 * @tparam TSeq Transfer sequence type.
 *
 * @note Requires all transfers in a transfer sequence to be allocated with the
 * number of iso packets valid for a given `iso_packets` value.
 */
template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
constexpr auto
prefill_isochronous (TSeq const &tfer_seq, endpoint<EpDirection> ep, int iso_packets,
                     std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
{
    auto it = detail::transfer_begin(tfer_seq);
    for (; it != detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_fill_iso_transfer(tfer, ep.devh(), ep.addr(), nullptr, 0, iso_packets, nullptr,
                                 nullptr, timeout_ms.count());
    }
}

/**
 * @ingroup transfer
 *
 * @brief Prefills transfer sequence as bulk stream transfers.
 * @tparam TSeq Transfer sequence type.
 *
 * @note Requires `libusb_alloc_streams` to be called for a given `stream_id`.
 */
template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
constexpr auto
prefill_bulk_stream (TSeq const &tfer_seq, endpoint<EpDirection> ep, uint32_t stream_id,
                     std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
{
    auto it = detail::transfer_begin(tfer_seq);
    for (; it != detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_fill_bulk_stream_transfer(tfer, ep.devh(), ep.addr(), stream_id, nullptr, 0, nullptr,
                                         nullptr, timeout_ms.count());
    }
}

} // namespace co_usb::transfer
