#pragma once

#include "co_usb/detail/left_state.hpp"
#include "co_usb/device_triplet.hpp"
#include <libusb-1.0/libusb.h>
#include <memory>
#include <memory_resource>

namespace co_usb
{

struct device_acceptor;

/**
 * @brief Signals hotplug device departure
 *
 * @details Signaling mechanism similar to but not based on @ref std::stop_source that owns a LEFT
 * callback that requests the stop on stop_source which is translated to device_left() method.
 *
 * @note The callback is deregistered when:
 * @li A. The callback was successfully fired
 * @li B. The callback did not fire but the dtor was called
 *
 * @note Signal state is stored on the heap to keep stable address for callback across moves.
 * Use raw @ref hotplug_awaitable if this is unnacceptable.
 *
 * @see device_acceptor
 */
struct left_signal
{
    friend struct left_token;
    friend struct co_usb::device_acceptor;

    ~left_signal() noexcept;

    left_signal(const left_signal &);
    left_signal &operator=(const left_signal &);
    left_signal(left_signal &&);
    left_signal &operator=(left_signal &&);

    /**
     * @returns true if device was detached
     */
    bool device_left() const;

    /**
     * @brief disarms a signal and deregisters a callback.
     *
     * @details Fully disarms the signal, making it unable to be fired if it wasn't before that
     * by deregistering the callback. If the signal was fired, this function does absolutely
     * nothing.
     *
     * @returns true if the signal was disarmed whether from calling this function or by callback
     * firing.
     */
    bool disarm();

  private:
    /**
     * @brief Constructs the object and creates and registers a callback with pointer to self as
     * user_data option
     */
    left_signal(libusb_context *ctx, device_triplet triplet,
                std::pmr::memory_resource *memory_resource = std::pmr::get_default_resource());

  private:
    std::shared_ptr<detail::left_state> m_state_ptr; // stable heap address
};

struct left_token
{
    explicit left_token(co_usb::left_signal &signal);

    /**
     * @returns true if device was detached
     */
    bool device_left() const;

  private:
    std::shared_ptr<detail::left_state> m_state_ptr;
};

} // namespace co_usb
