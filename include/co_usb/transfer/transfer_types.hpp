#pragma once

#include "co_usb/raii.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/transfer_awaitable.hpp"
#include <boost/capy/buffers.hpp>
#include <boost/capy/io_task.hpp>
#include <chrono>
#include <libusb-1.0/libusb.h>

namespace co_usb
{

template <endpoint_type EpType, ep_direction Direction> struct basic_transfer
{
    explicit basic_transfer (int iso_packets = 0)
        : m_tfer{libusb_alloc_transfer(iso_packets), libusb_free_transfer}
    {
        if (!m_tfer)
            throw std::bad_alloc{};
    }

    constexpr endpoint_type ep_type () const noexcept
    {
        return EpType;
    }

    auto raw () const noexcept
    {
        return m_tfer.get();
    }

    [[nodiscard]] boost::capy::io_task<size_t> read_some (boost::capy::mutable_buffer buf)
        requires(Direction == ep_direction::in || Direction == ep_direction::both)
    {
        auto tfer    = m_tfer.get();
        tfer->buffer = (uint8_t *)buf.data();
        tfer->length = buf.size();
        co_return co_await transfer_awaitable{tfer};
    }

    boost::capy::io_task<size_t> write_some (boost::capy::const_buffer buf)
        requires(Direction == ep_direction::out || Direction == ep_direction::both)
    {
        auto tfer    = m_tfer.get();
        tfer->buffer = (uint8_t *)buf.data();
        tfer->length = buf.size();
        co_return co_await transfer_awaitable{tfer};
    }

  protected:
    unique_transfer m_tfer;
};

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

} // namespace co_usb
