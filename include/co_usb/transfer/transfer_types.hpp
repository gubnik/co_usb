#pragma once

#include "co_usb/transfer/detail/basic_transfer.hpp"

namespace co_usb
{

/**
 * @brief Transfer type for control transfers
 */
template <size_t Preallocated = 1>
struct control_transfer
    : public detail::basic_transfer<endpoint_type::control, endpoint_direction::both, Preallocated>
{
    template <detail::DeviceHandleSource DevSourceTy>
    explicit control_transfer (boost::capy::executor_ref exec, DevSourceTy &&dev_source,
                               std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::basic_transfer<endpoint_type::control, endpoint_direction::both, Preallocated>(
              exec)
    {
        for (auto &ptr : this->m_tfers)
        {
            libusb_fill_control_transfer(
                ptr.get(), detail::device_handle_of(std::forward<DevSourceTy>(dev_source)), nullptr,
                nullptr, nullptr, timeout_ms.count());
        }
    }
};

/**
 * @brief Transfer type for bulk transfers
 */
template <endpoint_direction Dir, size_t Preallocated = 1>
    requires(Dir != endpoint_direction::both)
struct bulk_transfer : public detail::basic_transfer<endpoint_type::bulk, Dir, Preallocated>
{
    explicit bulk_transfer (boost::capy::executor_ref exec, endpoint<Dir> ep,
                            std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::basic_transfer<endpoint_type::bulk, Dir, Preallocated>(exec)
    {
        for (auto &ptr : this->m_tfers)
        {
            libusb_fill_bulk_transfer(ptr.get(), ep.devh(), ep.addr(), nullptr, 0, nullptr, nullptr,
                                      timeout_ms.count());
        }
    }
};

/**
 * @brief Transfer type for interrupt transfers
 */
template <endpoint_direction Dir, size_t Preallocated = 1>
    requires(Dir != endpoint_direction::both)
struct interrupt_transfer
    : public detail::basic_transfer<endpoint_type::interrupt, Dir, Preallocated>
{
    explicit interrupt_transfer (
        boost::capy::executor_ref exec, endpoint<Dir> ep,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::basic_transfer<endpoint_type::interrupt, Dir, Preallocated>(exec)
    {
        for (auto &ptr : this->m_tfers)
        {
            libusb_fill_interrupt_transfer(ptr.get(), ep.devh(), ep.addr(), nullptr, 0, nullptr,
                                           nullptr, timeout_ms.count());
        }
    }
};

/**
 * @brief Transfer type for isochronous transfers
 */
template <endpoint_direction Dir, size_t Preallocated = 1>
    requires(Dir != endpoint_direction::both)
struct isochronous_transfer
    : public detail::basic_transfer<endpoint_type::isochronous, Dir, Preallocated>
{
    explicit isochronous_transfer (
        boost::capy::executor_ref exec, endpoint<Dir> ep, int iso_num,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::basic_transfer<endpoint_type::isochronous, Dir, Preallocated>(exec, iso_num)
    {
        for (auto &ptr : this->m_tfers)
        {
            libusb_fill_iso_transfer(ptr.get(), ep.devh(), ep.addr(), nullptr, 0, iso_num, nullptr,
                                     nullptr, timeout_ms.count());
        }
    }
};

/**
 * @brief Transfer type for bulk stream transfers
 */
template <endpoint_direction Dir, size_t Preallocated = 1>
    requires(Dir != endpoint_direction::both)
struct bulk_stream_transfer
    : public detail::basic_transfer<endpoint_type::bulk_stream, Dir, Preallocated>
{
    explicit bulk_stream_transfer (
        boost::capy::executor_ref exec, endpoint<Dir> ep, uint32_t stream_id,
        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0})
        : detail::basic_transfer<endpoint_type::bulk_stream, Dir, Preallocated>(exec)
    {
        for (auto &ptr : this->m_tfers)
        {
            libusb_fill_bulk_stream_transfer(ptr.get(), ep.devh(), ep.addr(), stream_id, nullptr, 0,
                                             nullptr, nullptr, timeout_ms.count());
        }
    }
};

} // namespace co_usb
