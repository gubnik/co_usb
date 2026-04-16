#pragma once

#include <libusb-1.0/libusb.h>

namespace co_usb
{

struct unique_context
{
    using raw = libusb_context *;
    unique_context ()
    {
        libusb_init(&m_ctx);
    }

    unique_context (raw devh) : m_ctx(devh)
    {
    }

    unique_context(const unique_context &)            = delete;
    unique_context &operator=(const unique_context &) = delete;

    unique_context (unique_context &&other)
    {
        release();
        m_ctx       = other.m_ctx;
        other.m_ctx = nullptr;
    }

    unique_context &operator=(unique_context &&other)
    {
        release();
        m_ctx       = other.m_ctx;
        other.m_ctx = nullptr;
        return *this;
    }

    ~unique_context ()
    {
        release();
    }

    void release ()
    {
        if (m_ctx)
            libusb_exit(m_ctx);
        m_ctx = nullptr;
    }

    auto get () noexcept
    {
        return m_ctx;
    }

    const auto get () const noexcept
    {
        return m_ctx;
    }

    operator bool () const noexcept
    {
        return (bool)m_ctx;
    }

  private:
    raw m_ctx = nullptr;
};

} // namespace co_usb
