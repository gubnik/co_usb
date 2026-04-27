#pragma once

#include "co_usb/hotplug/device_ref.hpp"
#include <boost/capy/io_task.hpp>
#include <libusb-1.0/libusb.h>
namespace co_usb
{

/**
 * @brief Wrapper around raw @ref hotplug_awaitable
 *
 * @details This is a convenience wrapper and sarcifices control, namely ability to change
 * events and flags parameters for a better interface matching that of Corosio and Asio.
 *
 * @note maybe not needed to be a class, kept as a class for compatibility reasons
 */
struct device_acceptor
{
    explicit device_acceptor(libusb_context *ctx) noexcept;

    /**
     * @brief Wrapper aroung hotplug_awaitable with events=LIBUSB_HOTPLUG_DEVICE_ARRIVED and
     * flags=LIBUSB_HOTPLUG_ENUMERATE
     *
     * @returns @ref std::error_code and device_ref
     */
    boost::capy::io_task<device_ref> accept(int vid, int pid, int dev_class);

  private:
    libusb_context *m_ctx;
};

/**
 * @brief Free function version of @ref device_acceptor::accept
 */
boost::capy::io_task<device_ref> accept(libusb_context *ctx, int vid, int pid, int dev_class);

} // namespace co_usb
