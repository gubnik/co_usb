/**
 * @file event_handler.hpp
 * @brief Specification and implementation of EventHandler concept.
 */

#pragma once

#include "co_usb/usb_error.hpp"
#include <atomic>
#include <libusb.h>
#include <stop_token>
#include <thread>
#include <utility>

namespace co_usb::detail
{

/**
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
 * dependency, I also abstein from including such mechanism, so the idea is left as an exercise for
 * the reader :-). Feel free to submit a PR.
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

/**
 * @brief Trivial single-threaded libusb event handler.
 *
 * This handler starts a background loop that repeatedly calls
 * `libusb_handle_events_timeout()` until `std::stop_token` requests stop.
 */
struct trivial_event_handler
{
    /**
     * @brief Start the libusb event processing loop.
     * @param usb_ctx libusb context to handle events for.
     * @param stop Stop token used to end the loop.
     * @return `true` on successful start.
     */
    auto start (libusb_context *usb_ctx, std::stop_token stop) -> bool
    {
        auto handle_fn = [usb_ctx, stop]
        {
            const timeval ctv = {.tv_sec = 0, .tv_usec = 10'000};
            while (!stop.stop_requested())
            {
                timeval tv = ctv;
                auto r     = libusb_handle_events_timeout(usb_ctx, &tv);
                if (r != LIBUSB_SUCCESS)
                {
                    throw std::system_error{make_usb_error_code(static_cast<usb_error>(r))};
                }
            }
        };
        m_thread = std::thread{std::move(handle_fn)};
        return true;
    }

    /**
     * @brief Increment internal reference count.
     *
     * This implementation is a no-op.
     */
    auto ref () noexcept
    {
        // no-op, there is nothing to ref
    }

    /**
     * @brief Decrement internal reference count.
     *
     * This implementation is a no-op.
     */
    auto unref () noexcept
    {
        // no-op, there is nothing to ref
    }

    /**
     * @brief Stop the event loop and wait for the thread to finish.
     *
     * Must block until handling is stopped.
     */
    auto stop ()
    {
        if (m_thread.joinable())
            m_thread.join();
    }

    /**
     * @brief Stop the event thread on destruction.
     */
    ~trivial_event_handler ()
    {
        stop();
    }

  private:
    std::thread m_thread;
};

static_assert(EventHandler<trivial_event_handler>, "Not a proper usb event handler");
static_assert(EventHandler<trivial_event_handler>, "Not a proper usb event handler");

/**
 * @brief libusb event handler with simple internal reference counting.
 *
 * This implementation runs a background loop calling
 * libusb_handle_events_timeout() until stop is requested *and* the internal
 * counter reaches zero.
 */
struct refcounted_event_handler
{
    /**
     * @brief Start the libusb event processing loop.
     * @param usb_ctx libusb context to handle events for.
     * @param stop Stop token used to end the loop.
     * @return true on successful start.
     */
    auto start (libusb_context *usb_ctx, std::stop_token stop) -> bool
    {
        try
        {
            auto handle_fn = [this, usb_ctx, stop]
            {
                const timeval ctv = {.tv_sec = 0, .tv_usec = 10'000};
                for (;;)
                {
                    timeval tv = ctv;
                    auto r     = libusb_handle_events_timeout(usb_ctx, &tv);
                    if (r != LIBUSB_SUCCESS)
                    {
                        throw std::system_error{
                            make_usb_error_code(static_cast<co_usb::usb_error>(r))};
                    }

                    // no work and cancellation was required - oblige and leave
                    if (stop.stop_requested() && m_counter.load(std::memory_order_acquire) == 0)
                    {
                        break;
                    }
                }
            };
            m_thread = std::thread{std::move(handle_fn)};
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    /**
     * @brief Increment the internal reference counter.
     */
    auto ref () noexcept
    {
        m_counter.fetch_add(1, std::memory_order_release);
    }

    /**
     * @brief Decrement the internal reference counter.
     */
    auto unref () noexcept
    {
        m_counter.fetch_sub(1, std::memory_order_release);
    }

    /**
     * @brief Stop the event loop and wait for the thread to finish.
     *
     * Blocks until handling is stopped.
     */
    auto stop ()
    {
        if (m_thread.joinable())
            m_thread.join();
    }

    /**
     * @brief Stop the event thread on destruction.
     */
    ~refcounted_event_handler ()
    {
        stop();
    }

  private:
    std::atomic_size_t m_counter{0};
    std::thread m_thread;
};

static_assert(EventHandler<refcounted_event_handler>, "Not a proper usb event handler");
static_assert(EventHandler<refcounted_event_handler>, "Not a proper usb event handler");

} // namespace co_usb::detail
