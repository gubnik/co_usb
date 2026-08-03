/**
 * @file driver_guard.hpp
 * @brief RAII wrapper for detaching and reattaching kernel driver.
 */

#pragma once

#include "co_usb/usb_error.hpp"
#include "co_usb/wrapper/detail/device_handle_source.hpp"
#include "co_usb/wrapper/detail/error_protocol.hpp"
#include "co_usb/wrapper/error_protocol.hpp"
#include <libusb.h>
#include <system_error>

namespace co_usb
{

/**
 * @ingroup wrapper
 *
 * @brief RAII wrapper for detaching and reattaching kernel driver.
 *
 * @details This type assumes the driver is already detached at the point of construction. The
 * destructor will attempt to release the interface and may throw if the attach operation fails.
 *
 * @note Satisfies @ref detail::DeviceHandleSource.
 *
 * @see co_usb::detach_driver
 */
struct driver_guard
{
    ~driver_guard () noexcept(false)
    {
        std::error_code ec;
        attach(ec);
        if (ec)
        {
            throw std::system_error{ec};
        }
    }

    driver_guard(driver_guard const &) = delete;
    driver_guard &operator=(driver_guard const &) = delete;

    driver_guard (driver_guard &&other) noexcept
        : m_devh(other.m_devh), m_interface_num(other.m_interface_num)
    {
        other.m_devh = nullptr;
    }

    driver_guard &operator=(driver_guard &&other)
    {
        std::error_code ec;
        if (this == &other)
        {
            return *this;
        }
        attach(ec);
        if (ec)
        {
            throw std::system_error{ec};
        }
        m_devh = other.m_devh;
        m_interface_num = other.m_interface_num;
        other.m_devh = nullptr;
        return *this;
    }

    /**
     * @brief Reattaches the detached kernel driver.
     *
     * @details Sets the error code if the release operation fails.
     */
    auto attach (std::error_code &ec) noexcept -> void
    {
        if (!m_devh)
        {
            return;
        }
        int r = libusb_attach_kernel_driver(m_devh, m_interface_num);
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

    explicit driver_guard (detail::DeviceHandleSource auto const &devh_src, int driver_guard_num)
        : m_devh(detail::device_handle_of(devh_src)), m_interface_num(driver_guard_num)
    {
    }

  private:
    libusb_device_handle *m_devh{nullptr};
    int m_interface_num;
};
static_assert(detail::DeviceHandleSource<driver_guard>, "Not a proper device handle source");

/**
 * @ingroup wrapper
 *
 * @brief Detaches the kernel driver and returns a @ref driver_guard or error.
 *
 * @note Guarantees exception safety.
 *
 * @tparam ErrorTy @ref detail::ErrorProtocol
 */
template <detail::ErrorProtocol<driver_guard> ErrTy>
auto detach_driver (detail::DeviceHandleSource auto const &devh_src, int interface_num,
                    ErrTy &&errp = as_exception()) noexcept ->
    typename ErrTy::template return_type<driver_guard>
{
    try
    {
        libusb_device_handle *devh = detail::device_handle_of(devh_src);

        int r = libusb_detach_kernel_driver(devh, interface_num);
        if (r != LIBUSB_SUCCESS)
        {
            return errp.template with_error<driver_guard>(
                make_usb_error_code(static_cast<usb_error>(r)));
        }
        return errp.template with_success<driver_guard>(devh, interface_num);
    }
    catch (...)
    {
        return errp.template with_error<driver_guard>(make_usb_error_code(usb_error::unknown));
    }
}

} // namespace co_usb
