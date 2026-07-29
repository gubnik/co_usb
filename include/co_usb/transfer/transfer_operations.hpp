#pragma once

#include "co_usb/transfer/detail/device_handle_source.hpp"
#include "co_usb/transfer/detail/iterator.hpp"
#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include <chrono>
#include <libusb.h>

namespace co_usb
{

template <detail::TransferSequence TSeq> constexpr auto cancel_transfer (TSeq const &tfer_seq)
{
    auto it = co_usb::detail::transfer_begin(tfer_seq);
    for (; it != co_usb::detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_cancel_transfer(tfer);
    }
}

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
constexpr auto
prefill_control_transfer (TSeq const &tfer_seq, endpoint<EpDirection> ep,
                          std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
{
    // TODO: C++26 contract_assert support
    // TODO: maybe make this an exception and not an assertion
    assert((ep.addr() == 0x00) && (EpDirection == endpoint_direction::out) &&
           "OUT control transfers must have an endpoint address of 0x00");
    assert((ep.addr() == 0x80) && (EpDirection == endpoint_direction::in) &&
           "IN control transfers must have an endpoint address of 0x80");
    auto it = co_usb::detail::transfer_begin(tfer_seq);
    for (; it != co_usb::detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_fill_control_transfer(tfer, detail::device_handle_of(ep), nullptr, nullptr, nullptr,
                                     timeout_ms.count());
    }
}

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
constexpr auto
prefill_bulk_transfer (TSeq const &tfer_seq, endpoint<EpDirection> ep,
                       std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
{
    auto it = co_usb::detail::transfer_begin(tfer_seq);
    for (; it != co_usb::detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_fill_bulk_transfer(tfer, detail::device_handle_of(ep), ep.addr(), nullptr, 0,
                                  nullptr, nullptr, timeout_ms.count());
    }
}

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
constexpr auto
prefill_interrupt_transfer (TSeq const &tfer_seq, endpoint<EpDirection> ep,
                            std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
{
    auto it = co_usb::detail::transfer_begin(tfer_seq);
    for (; it != co_usb::detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_fill_interrupt_transfer(tfer, detail::device_handle_of(ep), ep.addr(), nullptr, 0,
                                       nullptr, nullptr, timeout_ms.count());
    }
}

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
constexpr auto
prefill_iso_transfer (TSeq const &tfer_seq, endpoint<EpDirection> ep, int iso_packets,
                      std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
{
    auto it = co_usb::detail::transfer_begin(tfer_seq);
    for (; it != co_usb::detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_fill_iso_transfer(tfer, detail::device_handle_of(ep), ep.addr(), nullptr, 0,
                                 iso_packets, nullptr, nullptr, timeout_ms.count());
    }
}

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
constexpr auto
prefill_bulk_stream_transfer (TSeq const &tfer_seq, endpoint<EpDirection> ep, uint32_t stream_id,
                              std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
{
    auto it = co_usb::detail::transfer_begin(tfer_seq);
    for (; it != co_usb::detail::transfer_end(tfer_seq); it++)
    {
        libusb_transfer *tfer = detail::transfer_of(*it);
        libusb_fill_bulk_stream_transfer(tfer, detail::device_handle_of(ep), ep.addr(), stream_id,
                                         nullptr, 0, nullptr, nullptr, timeout_ms.count());
    }
}

} // namespace co_usb
