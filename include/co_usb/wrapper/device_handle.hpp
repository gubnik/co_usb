#pragma once

#include "co_usb/ev/detail/handler_service.hpp"
#include "co_usb/hotplug/device_triplet.hpp"
#include "co_usb/usb_error.hpp"
#include "co_usb/wrapper/detail/error_protocol.hpp"
#include "co_usb/wrapper/device_ref.hpp"
#include "co_usb/wrapper/error_protocol.hpp"
#include <boost/capy/ex/executor_ref.hpp>
#include <libusb.h>
#include <memory>
#include <version>

namespace co_usb
{

/**
 * @ingroup wrapper
 *
 * @brief RAII wrapper around libusb_device_handle*.
 *
 * @note Satisfies @ref detail::DeviceHandleSource.
 *
 * @details Wraps device handle and ensures its lifetime via internal unique pointer.
 * Destructor closes the device.
 */
struct device_handle
{

    device_handle(const device_handle &) = delete;
    device_handle(device_handle &&) = default;
    device_handle &operator=(const device_handle &) = delete;
    device_handle &operator=(device_handle &&) = default;

    explicit device_handle (libusb_device_handle *devh) : m_storage(devh, libusb_close)
    {
    }

    auto get () const noexcept -> libusb_device_handle *
    {
        return m_storage.get();
    }

    auto reset () noexcept
    {
        m_storage.reset();
    }

    auto release () noexcept -> libusb_device_handle *
    {
        return m_storage.release();
    }

    auto close () noexcept
    {
        m_storage.reset();
    }

  private:
    std::unique_ptr<libusb_device_handle, decltype(&libusb_close)> m_storage;
};

/**
 * @ingroup wrapper
 *
 * @brief Opens a device by triplet and returns a @ref device_handle or error.
 *
 * @note Guarantees exception safety.
 *
 * @tparam ErrorTy @ref detail::ErrorProtocol
 */
template <detail::ErrorProtocol<device_handle> ErrTy>
auto open_device (boost::capy::executor_ref exec, device_triplet triplet,
                  ErrTy &&errp = as_exception()) noexcept ->
    typename ErrTy::template return_type<device_handle>
{
    try
    {
        libusb_context *ctx = detail::get_handler_service(exec).usb_context();
        libusb_device_handle *devh = libusb_open_device_with_vid_pid(ctx, triplet.vid, triplet.pid);
        if (!devh)
        {
            return errp.template with_error<device_handle>(make_usb_error_code(usb_error::unknown));
        }
        return errp.template with_success<device_handle>(devh);
    }
    catch (...)
    {
        return errp.template with_error<device_handle>(make_usb_error_code(usb_error::unknown));
    }
}

/**
 * @ingroup wrapper
 *
 * @brief Opens a device from a @ref device_ref and returns a @ref device_handle or error.
 *
 * @note Guarantees exception safety.
 *
 * @tparam ErrorTy @ref detail::ErrorProtocol
 */
template <detail::ErrorProtocol<device_handle> ErrTy>
auto open_device (device_ref dev_ref, ErrTy &&errp = as_exception()) noexcept ->
    typename ErrTy::template return_type<device_handle>
{
    try
    {
        libusb_device_handle *devh;
        int r = libusb_open(dev_ref.get(), &devh);
        if (r != LIBUSB_SUCCESS)
        {
            return errp.template with_error<device_handle>(
                make_usb_error_code(static_cast<usb_error>(r)));
        }
        return errp.template with_success<device_handle>(devh);
    }
    catch (...)
    {
        return errp.template with_error<device_handle>(make_usb_error_code(usb_error::unknown));
    }
}

} // namespace co_usb
