#pragma once

#include "co_usb/raii.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/transfer_awaitable.hpp"
#include <array>
#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/buffer_param.hpp>
#include <boost/capy/ex/this_coro.hpp>
#include <boost/capy/io_task.hpp>
#include <boost/capy/when_all.hpp>
#include <chrono>
#include <cstring>
#include <iterator>
#include <libusb.h>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace co_usb
{

template <typename AB, ep_direction Direction>
concept transfer_buffer =
    (boost::capy::ConstBufferSequence<AB> || boost::capy::MutableBufferSequence<AB>) &&
    (Direction == ep_direction::both ||
     (boost::capy::ConstBufferSequence<AB> && Direction == ep_direction::out) ||
     (boost::capy::MutableBufferSequence<AB> && Direction == ep_direction::in));

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
template <endpoint_type EpType, ep_direction Direction, size_t Preallocated = 1>
    requires(Preallocated > 0)
struct basic_transfer
{
    explicit basic_transfer (int iso_packets = 0)
        : m_tfers{
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

    constexpr auto direction () const noexcept -> ep_direction
    {
        return Direction;
    }

    constexpr auto ep_type () const noexcept -> endpoint_type
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
    auto read_some (MB buffers) -> boost::capy::io_task<size_t>
        requires(Direction == ep_direction::in || Direction == ep_direction::both)
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
    auto write_some (CB buffers) -> boost::capy::io_task<size_t>
        requires(Direction == ep_direction::out || Direction == ep_direction::both)
    {
        return submit(buffers);
    }

  private:
    template <transfer_buffer<Direction> AnyBufferSequence>
    auto submit (AnyBufferSequence buffers) -> boost::capy::io_task<size_t>
    {
        using buffer_t = boost::capy::buffer_type<AnyBufferSequence>;

        if constexpr (std::is_convertible_v<AnyBufferSequence, buffer_t>)
        {
            buffer_t buf = buffers;
            auto tfer    = m_tfers[0].get();
            tfer->buffer = reinterpret_cast<uint8_t *>(const_cast<void *>(buf.data()));
            tfer->length = buf.size();
            co_return co_await transfer_awaitable{tfer};
        }
        else
        {
            const size_t buffers_given = boost::capy::buffer_length(buffers);
            const size_t pool          = m_tfers.size();
            size_t total               = 0;
            size_t remaining           = buffers_given;
            auto it                    = std::ranges::begin(buffers);
            std::array<transfer_awaitable, Preallocated> ops;
            while (remaining > 0)
            {
                const size_t count = std::min(pool, remaining);
                for (size_t i = 0; i < count; ++i)
                {
                    buffer_t buf = *it;
                    ++it;
                    libusb_transfer *tfer = m_tfers[i].get();
                    // de facto const for OUT but we have to cast away const anyway
                    tfer->buffer = static_cast<uint8_t *>(const_cast<void *>(buf.data()));
                    tfer->length = buf.size();

                    ops[i] = tfer;
                }

                auto [ec, vec] =
                    co_await boost::capy::when_all(ops | std::ranges::views::take(count));
                total += std::accumulate(vec.begin(), vec.end(), size_t{0});

                if (ec)
                {
                    co_return {ec, total};
                }
                remaining -= count;
            }
            co_return {{}, total};
        }
    }

  protected:
    std::array<unique_transfer, Preallocated> m_tfers;
};

/**
 * @brief Transfer type for control transfers
 */
template <size_t Preallocated = 1>
struct control_transfer
    : public basic_transfer<endpoint_type::control, ep_direction::both, Preallocated>
{
    explicit control_transfer (libusb_device_handle *devh,
                               std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : basic_transfer<endpoint_type::control, ep_direction::both>()
    {
        for (auto &ptr : this->m_tfers)
        {
            libusb_fill_control_transfer(ptr.get(), devh, nullptr, nullptr, nullptr,
                                         timeout_ms.count());
        }
    }
};

/**
 * @brief Transfer type for bulk transfers
 */
template <ep_direction Dir, size_t Preallocated = 1>
    requires(Dir != ep_direction::both)
struct bulk_transfer : public basic_transfer<endpoint_type::bulk, Dir, Preallocated>
{
    explicit bulk_transfer (endpoint<Dir> ep,
                            std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : basic_transfer<endpoint_type::bulk, Dir>()
    {
        for (auto &ptr : this->m_tfers)
        {
            libusb_fill_bulk_transfer(ptr.get(), ep.dev(), ep.addr(), nullptr, 0, nullptr, nullptr,
                                      timeout_ms.count());
        }
    }
};

/**
 * @brief Transfer type for interrupt transfers
 */
template <ep_direction Dir, size_t Preallocated = 1>
    requires(Dir != ep_direction::both)
struct interrupt_transfer : public basic_transfer<endpoint_type::interrupt, Dir, Preallocated>
{
    explicit interrupt_transfer (
        endpoint<Dir> ep, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : basic_transfer<endpoint_type::interrupt, Dir>()
    {
        for (auto &ptr : this->m_tfers)
        {
            libusb_fill_interrupt_transfer(ptr.get(), ep.dev(), ep.addr(), nullptr, 0, nullptr,
                                           nullptr, timeout_ms.count());
        }
    }
};

/**
 * @brief Transfer type for isochronous transfers
 */
template <ep_direction Dir, size_t Preallocated = 1>
    requires(Dir != ep_direction::both)
struct isochronous_transfer : public basic_transfer<endpoint_type::isochronous, Dir, Preallocated>
{
    explicit isochronous_transfer (
        endpoint<Dir> ep, int iso_num,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : basic_transfer<endpoint_type::isochronous, Dir>(iso_num)
    {
        for (auto &ptr : this->m_tfers)
        {
            libusb_fill_iso_transfer(ptr.get(), ep.dev(), ep.addr(), nullptr, 0, iso_num, nullptr,
                                     nullptr, timeout_ms.count());
        }
    }
};

/**
 * @brief Transfer type for bulk stream transfers
 */
template <ep_direction Dir, size_t Preallocated = 1>
    requires(Dir != ep_direction::both)
struct bulk_stream_transfer : public basic_transfer<endpoint_type::bulk_stream, Dir, Preallocated>
{
    explicit bulk_stream_transfer (
        endpoint<Dir> ep, uint32_t stream_id,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : basic_transfer<endpoint_type::bulk_stream, Dir>()
    {
        for (auto &ptr : this->m_tfers)
        {
            libusb_fill_bulk_stream_transfer(ptr.get(), ep.dev(), ep.addr(), stream_id, nullptr, 0,
                                             nullptr, nullptr, timeout_ms.count());
        }
    }
};

} // namespace co_usb
