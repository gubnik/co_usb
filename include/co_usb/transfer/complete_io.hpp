#pragma once

#include "co_usb/transfer/detail/any_buffer_sequence.hpp"
#include "co_usb/transfer/detail/complete_sequence_awaitable.hpp"
#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/stream.hpp"
#include "co_usb/transfer/transfer_operations.hpp"
#include <boost/capy/io_task.hpp>
#include <memory_resource>

namespace co_usb::transfer
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
 * @par Transfer sequence guarantee
 *
 * Transfer sequence is expected to outlive an instance of this type which uses it.
 * Deallocating, modifying and replacing transfers while an asynchronous operation is
 * pending is undefined behaviour unless it is a `cancel_transfer` call in which case the operation
 * is well-defined and will cause this particular transfer to be cancelled and cause an error
 * with @ref transfer_status `cancelled` value.
 *
 * @par Submission
 *
 * The submissions are done in a streaming manner, hence the name, where the N transfers from a
 * sequence will be reused for M buffers, in an order in which the transfers complete. This allows
 * to saturate the USB controller ring with large enough set of buffers. In practice, however, the
 * amount of transfers provided has diminishing returns due to internal libusb mutex and ring
 * oversaturation, and after a certain points using more transfers will be slower than using fewer.
 *
 * @par Event handler guarantee
 *
 * Submission also creates a proper cancellation callback based on a stop token propagated through
 * Capy's io_env protocol. The cancellation ONLY signals cancellation via `libusb_cancel_transfer`
 * for each transfer in a sequence. Actual handling of this signal is entirely dependent on the
 * event handler consuming it. To prevent the event handler from shutting down prematurely the event
 * handler ref is held during the submission operation.
 *
 * @see raw_transfer_stream
 * @see event_handler_ref
 */
template <detail::TransferSequence TSeq> struct complete_io_base : public partial_io_base<TSeq>
{
    explicit complete_io_base (boost::capy::executor_ref exec, TSeq const &tfer_seq,
                               size_t single_transfer_limit,
                               std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        : partial_io_base<TSeq>(exec, tfer_seq), m_memres(memres),
          m_single_transfer_limit(single_transfer_limit)
    {
    }

    template <detail::AnyBufferSequence BuffersTy>
    auto complete_submit (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        auto exec = co_await boost::capy::this_coro::executor;
        auto stop = co_await boost::capy::this_coro::stop_token;

        const size_t transfers_used =
            std::min(boost::capy::buffer_length(buffers), this->m_view.size());
        std::stop_callback sb{stop, [&] ()
                              {
                                  const auto end = this->m_view.begin() + transfers_used;
                                  for (auto iter = this->m_view.begin(); iter != end; iter++)
                                  {
                                      libusb_transfer *tfer = detail::transfer_of(*iter);
                                      cancel_transfer(tfer);
                                  }
                              }};
        using awaitable_t = detail::complete_sequence_awaitable<TSeq, BuffersTy>;
        typename awaitable_t::await_state_t await_state(m_memres);
        co_return co_await awaitable_t(&await_state, &this->m_view, buffers, transfers_used,
                                       m_single_transfer_limit);
    }

  private:
    std::pmr::memory_resource *m_memres;
    size_t m_single_transfer_limit;
};

/**
 * @ingroup transfer
 *
 * @brief Interface complying with Boost.Http's ReadSource and WriteSink concepts
 *
 * @details ReadSource and WriteSink concepts were once defined in Capy. Since then they had
 * been moved to Boost.Http because they were only used in it and the libraries based on it.
 * This interface will be kept regardless to increase interoperability.
 *
 * @note EOF handling is ignored because USB does not always operate with EOF in mind.
 */
template <TransferSequence TSeq, endpoint_direction EpDirection> struct direction_complete_io_base
{
    explicit direction_complete_io_base (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, size_t single_transfer_limit,
        std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        : m_raw(exec, tfer_seq, single_transfer_limit, memres)
    {
    }

    template <boost::capy::MutableBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::in)
    inline auto read_some (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        return m_raw.submit(buffers);
    }

    template <boost::capy::MutableBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::in)
    inline auto read (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        return m_raw.complete_submit(buffers);
    }

    template <boost::capy::ConstBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::out)
    inline auto write_some (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        return m_raw.submit(buffers);
    }

    template <boost::capy::ConstBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::out)
    inline auto write (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        return m_raw.complete_submit(buffers);
    }

    template <boost::capy::ConstBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::out)
    inline auto write_eof (BuffersTy const &buffers) -> boost::capy::io_task<size_t>
    {
        return m_raw.complete_submit(buffers);
    }

    template <boost::capy::ConstBufferSequence BuffersTy>
        requires(EpDirection == endpoint_direction::out)
    inline auto write_eof () -> boost::capy::io_task<>
    {
        co_return {{}};
    }

  private:
    complete_io_base<TSeq> m_raw;
};

} // namespace detail

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct control_complete_io : detail::direction_complete_io_base<TSeq, EpDirection>
{
    explicit control_complete_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        size_t single_transfer_limit,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0},
        std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        : detail::direction_complete_io_base<TSeq, EpDirection>(exec, tfer_seq,
                                                                single_transfer_limit, memres)
    {
        prefill_control_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct bulk_complete_io : detail::direction_complete_io_base<TSeq, EpDirection>
{
    explicit bulk_complete_io (boost::capy::executor_ref exec, TSeq const &tfer_seq,
                               endpoint<EpDirection> endpoint, size_t single_transfer_limit,
                               std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0},
                               std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        : detail::direction_complete_io_base<TSeq, EpDirection>(exec, tfer_seq,
                                                                single_transfer_limit, memres)
    {
        prefill_bulk_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct interrupt_complete_io : detail::direction_complete_io_base<TSeq, EpDirection>
{
    explicit interrupt_complete_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        size_t single_transfer_limit,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0},
        std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        : detail::direction_complete_io_base<TSeq, EpDirection>(exec, tfer_seq,
                                                                single_transfer_limit, memres)
    {
        prefill_interrupt_transfer(tfer_seq, endpoint, timeout_ms);
    }
};

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct isochronous_complete_io : detail::direction_complete_io_base<TSeq, EpDirection>
{
    explicit isochronous_complete_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        int iso_packets, size_t single_transfer_limit,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0},
        std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        : detail::direction_complete_io_base<TSeq, EpDirection>(exec, tfer_seq,
                                                                single_transfer_limit, memres)
    {
        prefill_iso_transfer(tfer_seq, endpoint, iso_packets, timeout_ms);
    }
};

template <detail::TransferSequence TSeq, endpoint_direction EpDirection>
struct bulk_stream_complete_io : detail::direction_complete_io_base<TSeq, EpDirection>
{
    explicit bulk_stream_complete_io (
        boost::capy::executor_ref exec, TSeq const &tfer_seq, endpoint<EpDirection> endpoint,
        uint32_t stream_id, size_t single_transfer_limit,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0},
        std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        : detail::direction_complete_io_base<TSeq, EpDirection>(exec, tfer_seq,
                                                                single_transfer_limit, memres)
    {
        prefill_bulk_stream_transfer(tfer_seq, endpoint, stream_id, timeout_ms);
    }
};

template <detail::TransferSequence TSeq>
using control_sink = control_complete_io<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using control_source = control_complete_io<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq>
using bulk_sink = bulk_complete_io<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using bulk_source = bulk_complete_io<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq>
using interrupt_sink = interrupt_complete_io<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using interrupt_source = interrupt_complete_io<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq>
using isochronous_sink = isochronous_complete_io<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using isochronous_source = isochronous_complete_io<TSeq, endpoint_direction::in>;

template <detail::TransferSequence TSeq>
using bulk_stream_sink = bulk_stream_complete_io<TSeq, endpoint_direction::out>;
template <detail::TransferSequence TSeq>
using bulk_stream_source = bulk_stream_complete_io<TSeq, endpoint_direction::in>;

} // namespace co_usb::transfer
