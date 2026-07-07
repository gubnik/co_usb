#pragma once

#include "co_usb/transfer/detail/device_handle_source.hpp"
#include "co_usb/usb_error.hpp"
#include "co_usb/wrapper/detail/error_protocol.hpp"
#include <libusb.h>
#include <system_error>

namespace co_usb
{

struct interface
{
    ~interface () noexcept(false)
    {
        std::error_code ec;
        release(ec);
        if (ec)
        {
            throw std::system_error{ec};
        }
    }

    interface(interface const &)            = delete;
    interface &operator=(interface const &) = delete;

    interface (interface &&other) noexcept
        : m_devh(other.m_devh), m_interface_num(other.m_interface_num)
    {
        other.m_devh = nullptr;
    }

    interface &operator=(interface &&other)
    {
        std::error_code ec;
        if (this == &other)
        {
            return *this;
        }
        release(ec);
        if (ec)
        {
            throw std::system_error{ec};
        }
        m_devh          = other.m_devh;
        m_interface_num = other.m_interface_num;
        other.m_devh    = nullptr;
        return *this;
    }

    auto release (std::error_code &ec) noexcept -> void
    {
        if (!m_devh)
        {
            return;
        }
        int r = libusb_release_interface(m_devh, m_interface_num);
        if (r != LIBUSB_SUCCESS)
        {
            ec = make_usb_error_code(static_cast<usb_error>(r));
        }
        m_devh = nullptr;
    }

    auto devh () const noexcept -> libusb_device_handle *
    {
        return m_devh;
    }

    auto interface_num () const noexcept -> int
    {
        return m_interface_num;
    }

    explicit interface (detail::DeviceHandleSource auto const &devh_src, int interface_num)
        : m_devh(detail::device_handle_of(devh_src)), m_interface_num(interface_num)
    {
    }

  private:
    libusb_device_handle *m_devh{nullptr};
    int m_interface_num;
};
static_assert(detail::DeviceHandleSource<interface>, "Not a proper device handle source");

template <detail::ErrorProtocol<interface> ErrTy>
auto claim_interface (detail::DeviceHandleSource auto const &devh_src, int interface_num,
                      ErrTy &&errp) noexcept -> typename ErrTy::template return_type<interface>
{
    try
    {
        libusb_device_handle *devh = detail::device_handle_of(devh_src);

        int r = libusb_claim_interface(devh, interface_num);
        if (r != LIBUSB_SUCCESS)
        {
            return errp.template with_error<interface>(
                make_usb_error_code(static_cast<usb_error>(r)));
        }
        return errp.template with_success<interface>(devh, interface_num);
    }
    catch (...)
    {
        return errp.template with_error<interface>(make_usb_error_code(usb_error::unknown));
    }
}

} // namespace co_usb
