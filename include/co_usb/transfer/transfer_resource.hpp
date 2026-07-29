#pragma once

#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include <libusb.h>
#include <memory>

namespace co_usb
{

/**
 * @ingroup transfer
 *
 * @brief Generic RAII wrapper over a transfer allocation.
 *
 * @notes Satisfies TransferResource concept.
 */
struct transfer_resource
{
    explicit transfer_resource (int iso_packets = 0)
        : m_utfer(libusb_alloc_transfer(iso_packets), libusb_free_transfer)
    {
        if (!m_utfer)
        {
            throw std::bad_alloc{};
        }
    }

    auto get () const noexcept -> libusb_transfer *
    {
        return m_utfer.get();
    }

  private:
    using deleter_t = std::decay_t<decltype(libusb_free_transfer)>;
    std::unique_ptr<libusb_transfer, deleter_t> m_utfer{nullptr, libusb_free_transfer};
};

static_assert(detail::TransferResource<transfer_resource>, "Not a proper transfer resource");

} // namespace co_usb
