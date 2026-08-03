/**
 * @file event_handler.hpp
 * @brief Specification and implementation of EventHandler concept.
 */

#pragma once

#include <libusb.h>
#include <stop_token>
#include <utility>

namespace co_usb::ev::detail
{

/**
 * @ingroup event_handler
 *
 * @concept EventHandler
 * @tparam Ty Type satisfying the EventHandler contract.
 *
 * @brief Concept defining an event handler for libusb.
 *
 * @details Libusb makes it non-trivial to integrate its event handling into a common
 * fd polling loop. This abstraction allows pluggable event handling outside of executor
 * internals.
 *
 * An event handler is responsible for running libusb event processing (via
 * libusb_handle_events... and/or polling libusb fds), and for providing the thread(s)
 * used to run it.
 *
 * A type Ty satisfies EventHandler if it provides:
 * - Ty.start(libusb_context*, std::stop_token) -> bool
 * - Ty.ref() noexcept -> void
 * - Ty.unref() noexcept -> void
 * - Ty.stop() noexcept -> void (must block until stopped)
 *
 * @note As stated before, though non-trivial, it would indeed be possible to integrate libusb fds
 * with an existing event loop such as Corosio's. Since I do not intend on making Corosio a
 * dependency such handler is not included. Feel free to submit a PR.
 */
template <typename Ty>
concept EventHandler = requires(Ty handler) {
    {
        handler.start(std::declval<libusb_context *>(), std::declval<std::stop_token>())
    } -> std::same_as<bool>;
    { handler.ref() } noexcept -> std::same_as<void>;
    { handler.unref() } noexcept -> std::same_as<void>;
    { handler.stop() } -> std::same_as<void>;
};

} // namespace co_usb::ev::detail
