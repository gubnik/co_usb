/**
 * @file event_handler_ref.hpp
 * @brief Type-erased reference to any event handler.
 */

#pragma once

#include "co_usb/ev/any_event_handler.hpp"
#include "co_usb/ev/detail/event_handler.hpp"
#include <concepts>
#include <utility>

namespace co_usb::ev
{

/**
 * @ingroup event_handler
 *
 * @brief Type-erased reference to any event handler.
 *
 * @details The purpose of this type is to be held by an object within the lifetime of which
 * the event handler cannot be let to shutdown. An example of a situation where this would be
 * ill-formed is if a transfer is in flight, cancellation is requested, @ref
 * co_usb::transfer::cancel_transfer is called on it but the event loop has already finished its
 * shutdown and the transfer would never be cancelled nor is it completion callback would ever be
 * called.
 *
 * @note This type is copy constructible, copy assignable, move constructible and move assignable.
 * Use move to avoid refcount increment/decrement.
 *
 * @see any_event_handler
 */
struct event_handler_ref
{
    explicit inline event_handler_ref () noexcept
    {
    }

    template <detail::EventHandler HandlerTy>
        requires(!std::same_as<HandlerTy, event_handler_ref>)
    event_handler_ref(HandlerTy &handler) : m_original(&handler), m_vtable(make_vtable(handler))
    {
        ref();
    }

    template <detail::EventHandler HandlerTy>
        requires(!std::same_as<HandlerTy, event_handler_ref>)
    event_handler_ref &operator=(HandlerTy &handler)
    {
        if (m_original == &handler)
            return *this;
        m_original = &handler;
        m_vtable = make_vtable(handler);
        ref();
        return *this;
    }

    event_handler_ref (event_handler_ref const &other)
        : m_original(other.m_original), m_vtable(other.m_vtable)
    {
        ref();
    }

    event_handler_ref &operator=(event_handler_ref const &other)
    {
        if (this == &other)
            return *this;
        if (m_original)
            unref();
        m_original = other.m_original;
        m_vtable = other.m_vtable;
        ref();
        return *this;
    }

    event_handler_ref (event_handler_ref &&other) noexcept
        : m_original(other.m_original), m_vtable(other.m_vtable)
    {
        other.m_original = nullptr;
    }

    event_handler_ref &operator=(event_handler_ref &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        if (m_original)
            unref();
        m_original = other.m_original;
        m_vtable = other.m_vtable;
        other.m_original = nullptr;
        return *this;
    }

    ~event_handler_ref ()
    {
        unref();
    }

    inline auto valid () const noexcept -> bool
    {
        return m_original != nullptr;
    }

    auto start (libusb_context *usb_ctx, std::stop_token stop) -> bool
    {
        if (!m_original)
        {
            return false;
        }
        return m_vtable.start_fn(m_original, usb_ctx, std::move(stop));
    }

    auto ref () noexcept -> void
    {
        if (!m_original)
        {
            return;
        }
        return m_vtable.ref_fn(m_original);
    }

    auto unref () noexcept -> void
    {
        if (!m_original)
        {
            return;
        }
        return m_vtable.unref_fn(m_original);
    }

    auto stop () -> void
    {
        if (!m_original)
        {
            return;
        }
        return m_vtable.stop_fn(m_original);
    }

  private:
    struct vtable
    {
        using start_fn_t = auto (*)(void *, libusb_context *, std::stop_token) -> bool;
        using ref_fn_t = auto (*)(void *) -> void;
        using unref_fn_t = auto (*)(void *) -> void;
        using stop_fn_t = auto (*)(void *) -> void;

        start_fn_t start_fn{nullptr};
        ref_fn_t ref_fn{nullptr};
        unref_fn_t unref_fn{nullptr};
        stop_fn_t stop_fn{nullptr};
    };

    template <detail::EventHandler HandlerTy>
        requires(!std::same_as<HandlerTy, event_handler_ref>)
    auto make_vtable (HandlerTy &handler) -> vtable
    {
        (void)handler; // used for type deduction
        return vtable{
            .start_fn = +[] (void *orig, libusb_context *ctx, std::stop_token stop) -> bool
            { return static_cast<HandlerTy *>(orig)->start(ctx, stop); },
            .ref_fn = +[] (void *orig) -> void { return static_cast<HandlerTy *>(orig)->ref(); },
            .unref_fn = +[] (void *orig) -> void
            { return static_cast<HandlerTy *>(orig)->unref(); },
            .stop_fn = +[] (void *orig) -> void { return static_cast<HandlerTy *>(orig)->stop(); },
        };
    }

  private:
    void *m_original{nullptr};
    vtable m_vtable;
};

static_assert(detail::EventHandler<event_handler_ref>, "Not a proper USB event handler");

} // namespace co_usb::ev
