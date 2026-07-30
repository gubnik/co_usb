#pragma once

#include "co_usb/ev/detail/handler_service.hpp"
#include "co_usb/ev/event_handler_ref.hpp"
#include "co_usb/transfer/detail/any_buffer_sequence.hpp"
#include "co_usb/transfer/detail/sequence_awaitable.hpp"
#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/transfer_operations.hpp"
#include "co_usb/transfer/transfer_sequence_view.hpp"
#include <boost/capy/buffers.hpp>
#include <boost/capy/ex/executor_ref.hpp>
#include <boost/capy/ex/this_coro.hpp>
#include <boost/capy/io_task.hpp>

namespace co_usb
{

/**
 * @ingroup transfer
 *
 * @brief Zero-allocation wrapper around a transfer sequence enabling the use of asynchronous stream
 * operations on a given transfer sequence.
 *
 * @note Handles cancellation.
 *
 * @details This type is designed to be a barebones wrapper around raw machinery for tying
 * asynchronous transfers into coroutine ecosystem.
 *
 * The submissions are done in a streaming manner, hence the name, where the N transfers from a
 * sequence will be reused for M buffers, in an order in which the transfers complete. This allows
 * to saturate the USB controller ring with large enough set of buffers. In practice, however, the
 * amount of transfers provided has diminishing returns due to internal libusb mutex and ring
 * oversaturation, and after a certain points using more transfers will be slower than using fewer.
 *
 * Submission also creates a proper cancellation callback based on a stop token propagated through
 * Capy's io_env protocol. The cancellation ONLY signals cancellation via `libusb_cancel_transfer`
 * for each transfer in a sequence. Actual handling of this signal is entirely dependent on the
 * event handler consuming it. To prevent the event handler from shutting down prematurely the event
 * handler ref is held during the submission operation.
 *
 * @see event_handler_ref
 * @see detail::handler_service
 */
template <detail::TransferSequence TSeq> struct raw_transfer_stream
{
    explicit raw_transfer_stream (boost::capy::executor_ref exec, TSeq const &tfer_seq)
        : m_ev_ref(detail::get_handler_service(exec).handler()), m_view(tfer_seq)
    {
    }

    /**
     * @brief Submit a transfer sequence for a buffer sequence
     *
     * @returns result of the operation
     */
    template <detail::AnyBufferSequence BuffersTy>
    auto submit (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        auto exec = co_await boost::capy::this_coro::executor;
        auto stop = co_await boost::capy::this_coro::stop_token;

        const size_t transfers_used = std::min(boost::capy::buffer_length(buffers), m_view.size());
        std::stop_callback sb{stop, [&] ()
                              {
                                  const auto end = m_view.begin() + transfers_used;
                                  for (auto iter = m_view.begin(); iter != end; iter++)
                                  {
                                      libusb_transfer *tfer = detail::transfer_of(*iter);
                                      cancel_transfer(tfer);
                                  }
                              }};
        using awaitable_t = detail::sequence_awaitable<TSeq, BuffersTy>;
        typename awaitable_t::await_state_t await_state;
        co_return co_await awaitable_t(&await_state, &m_view, buffers, transfers_used);
    }

  private:
    event_handler_ref m_ev_ref;
    transfer_sequence_view<TSeq> m_view;
};

namespace detail
{
template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct direction_transfer_stream_base
{
    explicit direction_transfer_stream_base (boost::capy::executor_ref exec, TSeq const &tfer_seq)
        : m_stream(exec, tfer_seq)
    {
    }

    template <boost::capy::MutableBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::in)
    constexpr inline auto read_some (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        return m_stream.submit(buffers);
    }

    template <boost::capy::ConstBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::out)
    constexpr inline auto write_some (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        return m_stream.submit(buffers);
    }

  private:
    raw_transfer_stream<TSeq> m_stream;
};
} // namespace detail

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct control_transfer_stream : detail::direction_transfer_stream_base<TSeq, EpDirection>
{
    explicit control_transfer_stream (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_transfer_stream_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_control_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using control_transfer_write_stream = control_transfer_stream<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using control_transfer_read_stream = control_transfer_stream<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct bulk_transfer_stream : detail::direction_transfer_stream_base<TSeq, EpDirection>
{
    explicit bulk_transfer_stream (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_transfer_stream_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_bulk_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using bulk_transfer_write_stream = bulk_transfer_stream<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using bulk_transfer_read_stream = bulk_transfer_stream<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct interrupt_transfer_stream : detail::direction_transfer_stream_base<TSeq, EpDirection>
{
    explicit interrupt_transfer_stream (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_transfer_stream_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_interrupt_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using interrupt_transfer_write_stream = interrupt_transfer_stream<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using interrupt_transfer_read_stream = interrupt_transfer_stream<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct isochronous_transfer_stream : detail::direction_transfer_stream_base<TSeq, EpDirection>
{
    explicit isochronous_transfer_stream (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        int iso_packets, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_transfer_stream_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_iso_transfer(tfer_seq, endpoint, iso_packets, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using isochronous_transfer_write_stream =
    isochronous_transfer_stream<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using isochronous_transfer_read_stream = isochronous_transfer_stream<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct bulk_stream_transfer_stream : detail::direction_transfer_stream_base<TSeq, EpDirection>
{
    explicit bulk_stream_transfer_stream (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        uint32_t stream_id, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_transfer_stream_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_bulk_stream_transfer(tfer_seq, endpoint, stream_id, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using bulk_stream_transfer_write_stream =
    bulk_stream_transfer_stream<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using bulk_stream_transfer_read_stream = bulk_stream_transfer_stream<TSeq, endpoint_direction::in>;

} // namespace co_usb
