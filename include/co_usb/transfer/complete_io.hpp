#pragma once

#include "co_usb/ev/detail/handler_service.hpp"
#include "co_usb/ev/event_handler_ref.hpp"
#include "co_usb/transfer/detail/any_buffer_sequence.hpp"
#include "co_usb/transfer/detail/complete_sequence_awaitable.hpp"
#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/transfer_operations.hpp"
#include "co_usb/transfer/transfer_sequence_view.hpp"
#include <boost/capy/io_task.hpp>
#include <memory_resource>

namespace co_usb
{

namespace detail
{

/**
 * @ingroup transfer
 *
 * @brief Complete I/O adaptor for transfer sequences.
 *
 * @note Handles cancellation.
 *
 * @details Adoptor which allows to submit N transfers from a sequence for sequence of M buffers
 * until each buffer is fully processed. This operation is transfer-affine, meaning that a single
 * transfer will not jump to another buffer until it has finished filling the current one it holds.
 *
 * Similar to @ref raw_transfer_stream, submissions are done in a streaming manner.
 *
 * Similar to @ref raw_transfer_stream, the operation creates a stop callback for cancelling
 * in-flight transfers. Event handler ref is held for the entire lifetime of an object of this type.
 *
 * @see event_handler_ref
 * @see detail::handler_service
 */
template <TransferSequence TSeq> struct raw_transfer_complete_io_base
{
    explicit raw_transfer_complete_io_base (boost::capy::executor_ref exec, TSeq const &tfer_seq)
        : m_ev_ref(detail::get_handler_service(exec).handler()), m_view(tfer_seq)
    {
    }

    template <detail::AnyBufferSequence BuffersTy>
    auto complete_submit (BuffersTy const &buffers, size_t single_transfer_limit,
                          std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        -> boost::capy::io_task<size_t>
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
        using awaitable_t = detail::complete_sequence_awaitable<TSeq, BuffersTy>;
        typename awaitable_t::await_state_t await_state(memres);
        co_return co_await awaitable_t(&await_state, &m_view, buffers, transfers_used,
                                       single_transfer_limit);
    }

  private:
    event_handler_ref m_ev_ref;
    transfer_sequence_view<TSeq> m_view;
};

template <TransferSequence TSeq, endpoint_direction EpDirection>
struct direction_transfer_complete_io_base
{
    explicit direction_transfer_complete_io_base (boost::capy::executor_ref exec,
                                                  TSeq const &tfer_seq)
        : m_raw(exec, tfer_seq)
    {
    }

    template <boost::capy::MutableBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::in)
    constexpr inline auto
    read_all (BuffersTy const &buffers, size_t single_transfer_limit,
              std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        -> boost::capy::io_task<size_t>
    {
        return m_raw.complete_submit(buffers, single_transfer_limit, memres);
    }

    template <boost::capy::ConstBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::out)
    constexpr inline auto
    write_all (BuffersTy const &buffers, size_t single_transfer_limit,
               std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        -> boost::capy::io_task<size_t>
    {
        return m_raw.complete_submit(buffers, single_transfer_limit, memres);
    }

  private:
    raw_transfer_complete_io_base<TSeq> m_raw;
};

} // namespace detail

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct control_transfer_complete_io : detail::direction_transfer_complete_io_base<TSeq, EpDirection>
{
    explicit control_transfer_complete_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_transfer_complete_io_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_control_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using control_transfer_sink = control_transfer_complete_io<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using control_transfer_source = control_transfer_complete_io<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct bulk_transfer_complete_io : detail::direction_transfer_complete_io_base<TSeq, EpDirection>
{
    explicit bulk_transfer_complete_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_transfer_complete_io_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_bulk_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using bulk_transfer_sink = bulk_transfer_complete_io<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using bulk_transfer_source = bulk_transfer_complete_io<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct interrupt_transfer_complete_io
    : detail::direction_transfer_complete_io_base<TSeq, EpDirection>
{
    explicit interrupt_transfer_complete_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_transfer_complete_io_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_interrupt_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using interrupt_transfer_sink = interrupt_transfer_complete_io<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using interrupt_transfer_source = interrupt_transfer_complete_io<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct isochronous_transfer_complete_io
    : detail::direction_transfer_complete_io_base<TSeq, EpDirection>
{
    explicit isochronous_transfer_complete_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        int iso_packets, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_transfer_complete_io_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_iso_transfer(tfer_seq, endpoint, iso_packets, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using isochronous_transfer_sink = isochronous_transfer_complete_io<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using isochronous_transfer_source = isochronous_transfer_complete_io<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct bulk_stream_transfer_complete_io
    : detail::direction_transfer_complete_io_base<TSeq, EpDirection>
{
    explicit bulk_stream_transfer_complete_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        uint32_t stream_id, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::direction_transfer_complete_io_base<TSeq, EpDirection>(exec, tfer_seq)
    {
        prefill_bulk_stream_transfer(tfer_seq, endpoint, stream_id, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using bulk_stream_transfer_sink = bulk_stream_transfer_complete_io<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using bulk_stream_transfer_source = bulk_stream_transfer_complete_io<TSeq, endpoint_direction::in>;

} // namespace co_usb
