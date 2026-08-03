/**
 * @file handlers.hpp
 * @brief Collection of types implementing @ref co_usb::ev::detail::EventHandler.
 */

#pragma once

#include "co_usb/ev/detail/event_handler.hpp"
#include "co_usb/usb_error.hpp"
#include <atomic>
#include <thread>

namespace co_usb::ev
{
/**
 * @ingroup event_handler
 *
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
                auto r = libusb_handle_events_timeout(usb_ctx, &tv);
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

static_assert(detail::EventHandler<trivial_event_handler>, "Not a proper usb event handler");
static_assert(detail::EventHandler<trivial_event_handler>, "Not a proper usb event handler");

/**
 * @ingroup event_handler
 *
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
                    auto r = libusb_handle_events_timeout(usb_ctx, &tv);
                    if (r != LIBUSB_SUCCESS) [[unlikely]]
                    {
                        throw std::system_error{
                            make_usb_error_code(static_cast<co_usb::usb_error>(r))};
                    }

                    if (stop.stop_requested() && m_counter.load(std::memory_order_acquire) == 0)
                        [[unlikely]]
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

static_assert(detail::EventHandler<refcounted_event_handler>, "Not a proper usb event handler");
static_assert(detail::EventHandler<refcounted_event_handler>, "Not a proper usb event handler");

} // namespace co_usb::ev
