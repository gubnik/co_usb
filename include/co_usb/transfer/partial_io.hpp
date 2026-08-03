#pragma once

#include "co_usb/ev/detail/handler_service.hpp"
#include "co_usb/ev/event_handler_ref.hpp"
#include "co_usb/transfer/detail/any_buffer_sequence.hpp"
#include "co_usb/transfer/detail/sequence_awaitable.hpp"
#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/operations.hpp"
#include "co_usb/transfer/sequence_view.hpp"
#include <boost/capy/buffers.hpp>
#include <boost/capy/ex/executor_ref.hpp>
#include <boost/capy/ex/this_coro.hpp>
#include <boost/capy/io_task.hpp>

namespace co_usb::transfer
{

namespace detail
{
/**
 * @ingroup transfer
 *
 * @brief Zero-allocation wrapper around a transfer sequence enabling the use of asynchronous
 * partial I/O operations on a given transfer sequence.
 *
 * @note Handles cancellation.
 *
 * @details This type is designed to be a barebones wrapper around raw machinery for tying
 * asynchronous transfers into the coroutine ecosystem.
 *
 * @par Transfer sequence lifetime guarantee
 *
 * Transfer sequence is expected to outlive an instance of this type which uses it.
 * Deallocating, modifying and replacing transfers while an asynchronous operation is
 * pending is undefined behaviour unless it is a `cancel_transfer` call in which case the operation
 * is well-defined and will cause this particular transfer to be cancelled and cause an error
 * with @ref transfer_status `cancelled` value.
 *
 * @par Submission order
 *
 * The submissions are done in a streaming manner, hence the name, where the N transfers from a
 * sequence will be reused for M buffers, in an order in which the transfers complete. This allows
 * to saturate the USB controller ring with large enough set of buffers. In practice, however, the
 * amount of transfers provided has diminishing returns due to internal libusb mutex and ring
 * oversaturation, and after a certain points using more transfers will be slower than using fewer.
 *
 * @par Event handler guarantee
 *
 * To prevent event handler from prematurely shutting down on stop request an @ref event_handler_ref
 * is held for the entire lifetime of an object of this type.
 *
 * @see ev::event_handler_ref
 * @see ev::detail::handler_service
 */
template <detail::TransferSequence TSeq> struct partial_io_base
{
    explicit partial_io_base (boost::capy::executor_ref exec, TSeq const &tfer_seq)
        : m_ev_ref(::co_usb::ev::detail::get_handler_service(exec).handler()), m_view(tfer_seq)
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

  protected:
    ev::event_handler_ref m_ev_ref;
    sequence_view<TSeq> m_view;
};

/**
 * @ingroup transfer
 *
 * @brief Provides Capy ReadStream-/WriteStream-compliant interface for partial transfer I/O.
 */
template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct direction_partial_io_base
{
    explicit direction_partial_io_base (boost::capy::executor_ref exec, TSeq const &tfer_seq)
        : m_stream(exec, tfer_seq)
    {
    }

    /**
     * @brief Performs partial read to a buffer sequence.
     *
     * @details May read less than the buffer size.
     */
    template <boost::capy::MutableBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::in)
    inline auto read_some (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        return m_stream.submit(buffers);
    }

    /**
     * @brief Performs partial read to a buffer sequence.
     *
     * @details May write less than the buffer size.
     */
    template <boost::capy::ConstBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::out)
    inline auto write_some (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        return m_stream.submit(buffers);
    }

  private:
    partial_io_base<TSeq> m_stream;
};
} // namespace detail

/**
 * @ingroup transfer
 *
 * @brief Prefills the transfer sequence as control transfers and provides
 * partial I/O operations.
 *
 * @note Control transfers are only valid for endpoints 0x00 (OUT) and 0x80 (IN).
 */
template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct control_partial_io : detail::direction_partial_io_base<TSeq, EpDirection>
{
    explicit control_partial_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_partial_io_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_control_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

/**
 * @ingroup transfer
 *
 * @brief Prefills the transfer sequence as bulk transfers and provides
 * partial I/O operations.
 */

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct bulk_partial_io : detail::direction_partial_io_base<TSeq, EpDirection>
{
    explicit bulk_partial_io (boost::capy::executor_ref exec, TSeq const &tfer_seq,
                              endpoint<EpDirection> endpoint,
                              std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_partial_io_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_bulk_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

/**
 * @ingroup transfer
 *
 * @brief Prefills the transfer sequence as interrupt transfers and provides
 * partial I/O operations.
 */
template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct interrupt_partial_io : detail::direction_partial_io_base<TSeq, EpDirection>
{
    explicit interrupt_partial_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_partial_io_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_interrupt_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

/**
 * @ingroup transfer
 *
 * @brief Prefills the transfer sequence as isochronous transfers and provides
 * partial I/O operations.
 *
 * @note Requires all transfers in a transfer sequence to be allocated with the
 * number of iso packets valid for a given `iso_packets` value.
 */

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct isochronous_partial_io : detail::direction_partial_io_base<TSeq, EpDirection>
{
    explicit isochronous_partial_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        int iso_packets, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_partial_io_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_iso_transfer(tfer_seq, endpoint, iso_packets, timeout_ms);
    }
};

/**
 * @ingroup transfer
 *
 * @brief Prefills the transfer sequence as bulk stream transfers and provides
 * partial I/O operations.
 *
 * @note Requires `libusb_alloc_streams` to be called for a given `stream_id`.
 */
template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct bulk_stream_partial_io : detail::direction_partial_io_base<TSeq, EpDirection>
{
    explicit bulk_stream_partial_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        uint32_t stream_id, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_partial_io_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_bulk_stream_transfer(tfer_seq, endpoint, stream_id, timeout_ms);
    }
};

/**
 * @ingroup transfer
 *
 * @brief Write specialization of @ref co_usb::transfer::control_partial_io.
 */
template <detail::TransferSequence TSeq>
using control_out = control_partial_io<TSeq, endpoint_direction::out>;

/**
 * @ingroup transfer
 *
 * @brief Read specialization of @ref co_usb::transfer::control_partial_io.
 */
template <detail::TransferSequence TSeq>
using control_in = control_partial_io<TSeq, endpoint_direction::in>;

/**
 * @ingroup transfer
 *
 * @brief Write specialization of @ref co_usb::transfer::bulk_partial_io.
 */
template <detail::TransferSequence TSeq>
using bulk_out = bulk_partial_io<TSeq, endpoint_direction::out>;

/**
 * @ingroup transfer
 *
 * @brief Read specialization of @ref co_usb::transfer::bulk_partial_io.
 */
template <detail::TransferSequence TSeq>
using bulk_in = bulk_partial_io<TSeq, endpoint_direction::in>;

/**
 * @ingroup transfer
 *
 * @brief Write specialization of @ref co_usb::transfer::interrupt_partial_io.
 */
template <detail::TransferSequence TSeq>
using interrupt_out = interrupt_partial_io<TSeq, endpoint_direction::out>;

/**
 * @ingroup transfer
 *
 * @brief Read specialization of @ref co_usb::transfer::interrupt_partial_io.
 */
template <detail::TransferSequence TSeq>
using interrupt_in = interrupt_partial_io<TSeq, endpoint_direction::in>;

/**
 * @ingroup transfer
 *
 * @brief Write specialization of @ref co_usb::transfer::isochronous_partial_io.
 */
template <detail::TransferSequence TSeq>
using isochronous_out = isochronous_partial_io<TSeq, endpoint_direction::out>;

/**
 * @ingroup transfer
 *
 * @brief Read specialization of @ref co_usb::transfer::isochronous_partial_io.
 */
template <detail::TransferSequence TSeq>
using isochronous_in = isochronous_partial_io<TSeq, endpoint_direction::in>;

/**
 * @ingroup transfer
 *
 * @brief Write specialization of @ref co_usb::transfer::bulk_stream_partial_io.
 */
template <detail::TransferSequence TSeq>
using bulk_stream_out = bulk_stream_partial_io<TSeq, endpoint_direction::out>;

/**
 * @ingroup transfer
 *
 * @brief Read specialization of @ref co_usb::transfer::bulk_stream_partial_io.
 */
template <detail::TransferSequence TSeq>
using bulk_stream_in = bulk_stream_partial_io<TSeq, endpoint_direction::in>;

} // namespace co_usb::transfer
