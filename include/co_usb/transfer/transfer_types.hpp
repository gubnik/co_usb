#pragma once

#include "co_usb/raii.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/transfer_awaitable.hpp"
#include <boost/capy/buffers.hpp>
#include <boost/capy/io_task.hpp>
#include <chrono>
#include <type_traits>

namespace co_usb
{

/**
 * @brief Base transfer type which provides Capy's ReadStream/WriteStream operations
 * depending on the direction of the endpoint provided.
 *
 * @note Prefer using concrete transfer types which are correctly prefilled on construction
 */
template <endpoint_type EpType, ep_direction Direction> struct basic_transfer
{
    explicit basic_transfer (int iso_packets = 0)
        : m_tfer{libusb_alloc_transfer(iso_packets), libusb_free_transfer}
    {
        if (!m_tfer)
            throw std::bad_alloc{};
    }

    consteval ep_direction direction () const noexcept
    {
        return Direction;
    }

    consteval endpoint_type ep_type () const noexcept
    {
        return EpType;
    }

    auto raw () const noexcept
    {
        return m_tfer.get();
    }

    /**
     * @brief Partial read from an endpoint.
     *
     * @details Provides a buffer to the transfer and submits it via @ref transfer_awaitable.
     * For buffer sequences that are ranges, does that N times for each of N buffers reusing the
     * internal transfer.
     *
     * @note Overrides the existing buffer, length, user_data and callback_fn of the internal
     * libusb_transfer.
     */
    template <boost::capy::MutableBufferSequence MB>
    boost::capy::io_task<size_t> read_some (MB buffers)
        requires(Direction == ep_direction::in || Direction == ep_direction::both)
    {
        if constexpr (std::is_convertible_v<MB, boost::capy::mutable_buffer>)
        {
            boost::capy::mutable_buffer buf = buffers;

            auto tfer    = m_tfer.get();
            tfer->buffer = (uint8_t *)buf.data();
            tfer->length = buf.size();
            co_return co_await transfer_awaitable{tfer};
        }
        size_t total = 0;
        for (boost::capy::mutable_buffer buf : buffers)
        {
            auto tfer    = m_tfer.get();
            tfer->buffer = (uint8_t *)buf.data();
            tfer->length = buf.size();
            auto [ec, n] = co_await transfer_awaitable{tfer};
            if (ec)
            {
                co_return {ec, total};
            }
            total += n;
        }
        co_return {{}, total};
    }

    /**
     * @brief Partial write to an endpoint.
     *
     * @details Provides a buffer to the transfer and submits it via @ref transfer_awaitable.
     * For buffer sequences that are ranges, does that N times for each of N buffers reusing the
     * internal transfer.
     *
     * @note Overrides the existing buffer, length, user_data and callback_fn of the internal
     * libusb_transfer.
     */
    template <boost::capy::ConstBufferSequence CB>
    boost::capy::io_task<size_t> write_some (CB buffers)
        requires(Direction == ep_direction::out || Direction == ep_direction::both)
    {
        if constexpr (std::is_convertible_v<CB, boost::capy::const_buffer>)
        {
            boost::capy::const_buffer buf = buffers;

            auto tfer    = m_tfer.get();
            tfer->buffer = (uint8_t *)buf.data();
            tfer->length = buf.size();
            co_return co_await transfer_awaitable{tfer};
        }
        size_t total = 0;
        for (boost::capy::const_buffer buf : buffers)
        {
            auto tfer    = m_tfer.get();
            tfer->buffer = (uint8_t *)buf.data();
            tfer->length = buf.size();
            auto [ec, n] = co_await transfer_awaitable{tfer};
            if (ec)
            {
                co_return {ec, total};
            }
            total += n;
        }
        co_return {{}, total};
    }

  protected:
    unique_transfer m_tfer;
};

/**
 * @brief Transfer type for control transfers
 */
struct control_transfer : public basic_transfer<endpoint_type::control, ep_direction::both>
{
    explicit control_transfer (libusb_device_handle *devh,
                               std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : basic_transfer<endpoint_type::control, ep_direction::both>()
    {
        libusb_fill_control_transfer(this->m_tfer.get(), devh, nullptr, nullptr, nullptr,
                                     timeout_ms.count());
    }
};

/**
 * @brief Transfer type for bulk transfers
 */
template <ep_direction Dir>
    requires(Dir != ep_direction::both)
struct bulk_transfer : public basic_transfer<endpoint_type::bulk, Dir>
{
    explicit bulk_transfer (endpoint<Dir> ep,
                            std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : basic_transfer<endpoint_type::bulk, Dir>()
    {
        libusb_fill_bulk_transfer(this->m_tfer.get(), ep.dev(), ep.addr(), nullptr, 0, nullptr,
                                  nullptr, timeout_ms.count());
    }
};

/**
 * @brief Transfer type for interrupt transfers
 */
template <ep_direction Dir>
    requires(Dir != ep_direction::both)
struct interrupt_transfer : public basic_transfer<endpoint_type::interrupt, Dir>
{
    explicit interrupt_transfer (
        endpoint<Dir> ep, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : basic_transfer<endpoint_type::interrupt, Dir>()
    {
        libusb_fill_interrupt_transfer(this->m_tfer.get(), ep.dev(), ep.addr(), nullptr, 0, nullptr,
                                       nullptr, timeout_ms.count());
    }
};

/**
 * @brief Transfer type for isochronous transfers
 */
template <ep_direction Dir>
    requires(Dir != ep_direction::both)
struct isochronous_transfer : public basic_transfer<endpoint_type::isochronous, Dir>
{
    explicit isochronous_transfer (
        endpoint<Dir> ep, int iso_num,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : basic_transfer<endpoint_type::isochronous, Dir>(iso_num)
    {
        libusb_fill_iso_transfer(this->m_tfer.get(), ep.dev(), ep.addr(), nullptr, 0, iso_num,
                                 nullptr, nullptr, timeout_ms.count());
    }
};

/**
 * @brief Transfer type for bulk stream transfers
 */
template <ep_direction Dir>
    requires(Dir != ep_direction::both)
struct bulk_stream_transfer : public basic_transfer<endpoint_type::bulk_stream, Dir>
{
    explicit bulk_stream_transfer (
        endpoint<Dir> ep, uint32_t stream_id,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : basic_transfer<endpoint_type::bulk_stream, Dir>()
    {
        libusb_fill_bulk_stream_transfer(this->m_tfer.get(), ep.dev(), ep.addr(), stream_id,
                                         nullptr, 0, nullptr, nullptr, timeout_ms.count());
    }
};

} // namespace co_usb
