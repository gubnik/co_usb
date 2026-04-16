#pragma once

#include <libusb-1.0/libusb.h>

namespace co_usb
{

struct unique_transfer
{
    using raw = libusb_transfer *;
    unique_transfer (int iso_transfers = 0)
    {
        m_tfer = libusb_alloc_transfer(iso_transfers);
    }

    unique_transfer (raw devh) : m_tfer(devh)
    {
    }

    unique_transfer(const unique_transfer &)            = delete;
    unique_transfer &operator=(const unique_transfer &) = delete;

    unique_transfer (unique_transfer &&other)
    {
        release();
        m_tfer       = other.m_tfer;
        other.m_tfer = nullptr;
    }

    unique_transfer &operator=(unique_transfer &&other)
    {
        release();
        m_tfer       = other.m_tfer;
        other.m_tfer = nullptr;
        return *this;
    }

    ~unique_transfer ()
    {
        release();
    }

    void release ()
    {
        if (m_tfer)
            libusb_free_transfer(m_tfer);
        m_tfer = nullptr;
    }

    auto get () noexcept
    {
        return m_tfer;
    }

    const auto get () const noexcept
    {
        return m_tfer;
    }

    operator bool () const noexcept
    {
        return (bool)m_tfer;
    }

  private:
    raw m_tfer = nullptr;
};

} // namespace co_usb
