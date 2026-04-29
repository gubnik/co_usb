#pragma once

#include <libusb-1.0/libusb.h>
#include <memory>

namespace co_usb
{

/**
 * @brief Signals hotplug device departure
 *
 * @details Signaling mechanism similar to but not based on @ref std::stop_source that owns a LEFT
 * callback that requests the stop on stop_source which is translated to device_left() method. This
 * class is not intended to be used on its own, see @ref device_acceptor.
 *
 * This is a single-fire signal, it will remain valid even after signalling but will not
 * convey meaningful information since the device could arrive once again. Moved-from signals
 * are considered to have been fired since the alternative (return false from a moved-from
 * @ref device_left()) would make an infinite loop trivial to write with just a single bad
 * std::move.
 *
 * Unlike @ref std::stop_source, this type does not have its own version of @ref std::stop_token,
 * this is a deliberate design choice to prevent misleading designs where one could
 * wrongly use objects of this type as a cancellation source instead of a specific signal which
 * would be semantically incoherent. You must still use a proper @ref std::stop_token for
 * cancellation and use this signal for SPECIFICALLY handling devices being detached.
 *
 * @note The callback is deregistered when:
 * @li A. The callback was successfully fired
 * @li B. The callback did not fire but the dtor was called
 *
 * @note Signal state is stored on the heap to keep stable address for callback across moves.
 * Use raw @ref hotplug_awaitable if this is unnacceptable.
 */
struct device_left_signal
{
    /**
     * @brief Constructs the object and creates and registers a callback with pointer to self as
     * user_data option
     */
    device_left_signal(libusb_context *ctx, int vid, int pid, int dev_class);
    ~device_left_signal() noexcept;

    device_left_signal(const device_left_signal &)            = delete; /* does not make sense */
    device_left_signal &operator=(const device_left_signal &) = delete; /* does not make sense */

    device_left_signal(device_left_signal &&);
    device_left_signal &operator=(device_left_signal &&);

    /**
     * @returns true if device was detached
     */
    bool device_left() const;

  private:
    std::unique_ptr<struct cb_data> m_data; // stable heap address
};

} // namespace co_usb
