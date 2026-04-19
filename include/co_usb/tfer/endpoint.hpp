#pragma once

#include <libusb-1.0/libusb.h>
#include <stdexcept>
namespace co_usb
{

enum class endpoint_type : uint8_t
{
    control = 0,
    bulk,
    interrupt,
    isochronous,
    bulk_stream,
};

enum class ep_direction
{
    out  = 0x00,
    in   = 0x80,
    both = 0xFF,
};

template <ep_direction Direction> struct endpoint
{
    endpoint (uint8_t ep, libusb_device_handle *devh) : m_ep(ep), m_devh(devh)
    {
        if constexpr (Direction == ep_direction::out)
        {
            if (ep > LIBUSB_ENDPOINT_IN)
            {
                throw std::invalid_argument{"Cannot use IN endpoint for OUT"};
            }
        }
        else if constexpr (Direction == ep_direction::in)
        {
            if (ep < LIBUSB_ENDPOINT_IN)
            {
                throw std::invalid_argument{"Cannot use OUT endpoint for IN"};
            }
        }
    }

    uint8_t addr () const noexcept
    {
        return m_ep;
    }

    auto dev () noexcept
    {
        return m_devh;
    }

    auto *dev () const noexcept
    {
        return m_devh;
    }

  private:
    uint8_t m_ep;
    libusb_device_handle *m_devh;
};

endpoint<ep_direction::in> ep_in(uint8_t ep, libusb_device_handle *devh);
endpoint<ep_direction::out> ep_out(uint8_t ep, libusb_device_handle *devh);

} // namespace co_usb
