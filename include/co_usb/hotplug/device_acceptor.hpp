#pragma once

#include "co_usb/hotplug/device_left_signal.hpp"
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
     * @brief Switches enumeration mode. Default is ON
     *
     * Passing true is equivalent to setting LIBUSB_HOTPLUG_ENUMERATE for flags.
     */
    void set_enumeration(bool do_enumerate) noexcept;

    /**
     * @returns current enumeration mode
     *
     * true is equivalent to LIBUSB_HOTPLUG_ENUMERATE.
     */
    bool do_enumerate() const noexcept;

    /**
     * @brief Wrapper aroung hotplug_awaitable with events=LIBUSB_HOTPLUG_DEVICE_ARRIVED and
     * flags=LIBUSB_HOTPLUG_ENUMERATE
     *
     * @returns @ref std::error_code and device_ref
     */
    boost::capy::io_task<device_ref> accept(int vid, int pid, int dev_class);

    /**
     * @brief Wrapper aroung hotplug_awaitable with events=LIBUSB_HOTPLUG_DEVICE_ARRIVED and
     * flags=LIBUSB_HOTPLUG_ENUMERATE that also registers a LIBUSB_HOTPLUG_DEVICE_LEFT callback
     * on successful arrival and exposes a signal that triggers on successful callback
     *
     * @note device_left_token is a single-use, do not attempt to store it for prolonged periods of
     * time
     *
     * @returns @ref std::error_code, @ref device_ref and @ref device_left_token
     */
    boost::capy::io_task<device_ref, device_left_signal> accept_with_left(int vid, int pid,
                                                                          int dev_class);

  private:
    libusb_context *m_ctx;
    bool m_do_enumerate = true;
};

} // namespace co_usb
