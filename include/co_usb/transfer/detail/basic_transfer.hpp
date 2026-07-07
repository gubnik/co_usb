#pragma once

#include "boost/capy/buffers.hpp"
#include "co_usb/ev/detail/handler_service.hpp"
#include "co_usb/ev/event_handler_ref.hpp"
#include "co_usb/transfer/detail/sliding_transfer_awaitable.hpp"
#include "co_usb/transfer/detail/transfer_awaitable.hpp"
#include "co_usb/transfer/detail/transfer_buffer.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include <boost/capy/ex/executor_ref.hpp>
#include <boost/capy/ex/this_coro.hpp>
#include <boost/capy/io_task.hpp>
#include <boost/capy/when_all.hpp>
#include <libusb.h>
#include <memory>
#include <stop_token>
#include <type_traits>

namespace co_usb::detail
{

/**
 * @brief Base transfer type which provides Capy's ReadStream/WriteStream operations
 * depending on the direction of the endpoint provided.
 *
 * @details Preallocates transfers on constructor.
 *
 * @note Awaiting more than one read_some/write_some per a single object of this type is disallowed.
 * Do not attempt to await from multiple threads explicitly, prefer using thread pool executors.
 *
 * @note Prefer using concrete transfer types which are correctly prefilled on construction
 */
template <endpoint_type EpType, endpoint_direction Direction, size_t Preallocated = 1>
    requires(
        Preallocated > 0 &&
        (Direction != endpoint_direction::both ||
         EpType ==
             endpoint_type::control)) // both is malformed for anything except control transfers
struct basic_transfer
{
  private:
    using unique_transfer =
        std::unique_ptr<libusb_transfer, std::decay_t<decltype(libusb_free_transfer)>>;

    struct stop_cb_t
    {
        libusb_transfer *tfer;
        void operator()()
        {
            libusb_cancel_transfer(tfer);
        }
    };

  public:
    explicit basic_transfer (boost::capy::executor_ref exec, int iso_packets = 0)
        : m_ev_handler_ref(get_handler_service(exec).handler()),
          m_tfers{
              [&]<size_t... I>(std::index_sequence<I...>)
              {
                  constexpr auto ctor = [] (int iso_packets)
                  {
                      auto tfer =
                          unique_transfer{libusb_alloc_transfer(iso_packets), libusb_free_transfer};
                      if (!tfer)
                          throw std::bad_alloc{};
                      return std::move(tfer);
                  };
                  return std::array<unique_transfer, Preallocated>{((void)I, ctor(iso_packets))...};
              }(std::make_index_sequence<Preallocated>{})}
    {
    }

    constexpr auto endpoint_direction () const noexcept -> endpoint_direction
    {
        return Direction;
    }

    constexpr auto endpoint_type () const noexcept -> endpoint_type
    {
        return EpType;
    }

    auto raw (size_t idx = 0) const
    {
        if (idx >= m_tfers.size())
        {
            throw std::out_of_range{"Transfer index out of range"};
        }
        return m_tfers[idx].get();
    }

    /**
     * @brief Partial read from an endpoint without transfer allocations.
     *
     * @details Provides a buffer to the transfer and submits it via @ref transfer_awaitable.
     * Uses only preallocated transfers and does not allocate new transfers.
     *
     * @note Overrides the existing buffer, length, user_data and callback_fn of the internal
     * libusb_transfer.
     */
    template <boost::capy::MutableBufferSequence MB>
    auto read_some (MB const &buffers) -> boost::capy::IoAwaitable auto
        requires(Direction == endpoint_direction::in || Direction == endpoint_direction::both)
    {
        return submit(buffers);
    }

    /**
     * @brief Partial write to an endpoint.
     *
     * @details Provides a buffer to the transfer and submits it via @ref transfer_awaitable.
     * Uses only preallocated transfers and does not allocate new transfers.
     *
     * @note Overrides the existing buffer, length, user_data and callback_fn of the internal
     * libusb_transfer.
     */
    template <boost::capy::ConstBufferSequence CB>
    auto write_some (CB const &buffers) -> boost::capy::IoAwaitable auto
        requires(Direction == endpoint_direction::out || Direction == endpoint_direction::both)
    {
        return submit(buffers);
    }

  private:
    template <transfer_buffer<Direction> AnyBufferSequence>
        requires(
            std::convertible_to<AnyBufferSequence, boost::capy::buffer_type<AnyBufferSequence>> &&
            !std::ranges::bidirectional_range<AnyBufferSequence>)
    auto submit (AnyBufferSequence const &buffers) -> boost::capy::io_task<size_t>
    {
        auto stop      = co_await boost::capy::this_coro::stop_token;
        using buffer_t = boost::capy::buffer_type<AnyBufferSequence>;

        buffer_t buf = buffers;
        auto tfer    = m_tfers[0].get();
        tfer->buffer = static_cast<uint8_t *>(const_cast<void *>(buf.data()));
        tfer->length = buf.size();
        transfer_awaitable::resumption_t res{};
        std::stop_callback stop_cb{stop, stop_cb_t{tfer}};
        co_return co_await transfer_awaitable{tfer, &res};
    }

    template <transfer_buffer<Direction> AnyBufferSequence>
        requires(
            !std::convertible_to<AnyBufferSequence, boost::capy::buffer_type<AnyBufferSequence>> &&
            std::ranges::bidirectional_range<AnyBufferSequence>)
    auto submit (AnyBufferSequence const &buffers) -> boost::capy::io_task<size_t>
    {
        auto stop         = co_await boost::capy::this_coro::stop_token;
        using buffer_t    = boost::capy::buffer_type<AnyBufferSequence>;
        using awaitable_t = sliding_transfer_awaitable<AnyBufferSequence>;
        using aw_state_t  = typename awaitable_t::await_state;
        aw_state_t aw_state(buffers, m_ev_handler_ref);
        std::array<libusb_transfer *, Preallocated> raw_tfers;
        for (size_t i = 0; i < Preallocated; i++)
        {
            raw_tfers[i] = m_tfers[i].get();
        }
        std::stop_callback stop_cb{
            stop, [&] ()
            {
                for (size_t i = 0; i < std::min(aw_state.total_buffers, m_tfers.size()); i++)
                {
                    libusb_cancel_transfer(m_tfers[i].get());
                }
            }};
        co_return co_await awaitable_t{aw_state, raw_tfers};
    }

  protected:
    event_handler_ref m_ev_handler_ref;
    std::array<unique_transfer, Preallocated> m_tfers;
};
} // namespace co_usb::detail
