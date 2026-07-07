#pragma once

#include "co_usb/ev/any_event_handler.hpp"
#include "co_usb/ev/detail/event_handler.hpp"
#include <concepts>

namespace co_usb
{

/**
 * @brief Type-erased reference to any event handler
 *
 * Performs RAII reference counting via handler's given functions and exposes
 * explicit ref()/unref() latches.
 */
struct event_handler_ref
{
    explicit event_handler_ref () noexcept
    {
    }

    template <detail::EventHandler HandlerTy>
        requires(!std::same_as<HandlerTy, event_handler_ref>)
    event_handler_ref(HandlerTy &handler)
        : m_original(&handler),
          m_vtable(vtable{
              .start_fn = +[] (void *orig, libusb_context *ctx, std::stop_token stop) -> bool
              { return static_cast<HandlerTy *>(orig)->start(ctx, stop); },
              .ref_fn = +[] (void *orig) -> void { return static_cast<HandlerTy *>(orig)->ref(); },
              .unref_fn = +[] (void *orig) -> void
              { return static_cast<HandlerTy *>(orig)->unref(); },
              .stop_fn = +[] (void *orig) -> void
              { return static_cast<HandlerTy *>(orig)->stop(); },
          })
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
        m_vtable   = vtable{
            .start_fn = +[] (void *orig, libusb_context *ctx, std::stop_token stop) -> bool
            { return static_cast<HandlerTy *>(orig)->start(ctx, stop); },
            .ref_fn   = +[] (void *orig) -> void { return static_cast<HandlerTy *>(orig)->ref(); },
            .unref_fn = +[] (void *orig) -> void
            { return static_cast<HandlerTy *>(orig)->unref(); },
            .stop_fn = +[] (void *orig) -> void { return static_cast<HandlerTy *>(orig)->stop(); },
        };
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
        m_vtable   = other.m_vtable;
        ref();
        return *this;
    }

    event_handler_ref (event_handler_ref &&other)
        : m_original(other.m_original), m_vtable(other.m_vtable)
    {
        other.m_original = nullptr;
    }

    event_handler_ref &operator=(event_handler_ref &&other)
    {
        if (this == &other)
        {
            return *this;
        }
        if (m_original)
            unref();
        m_original       = other.m_original;
        m_vtable         = other.m_vtable;
        other.m_original = nullptr;
        return *this;
    }

    ~event_handler_ref ()
    {
        if (!m_original)
        {
            return;
        }
        unref();
    }

    auto valid () const noexcept -> bool
    {
        return m_original;
    }

    auto start (libusb_context *usb_ctx, std::stop_token stop) -> bool
    {
        if (!m_original)
        {
            return false;
        }
        return m_vtable.start_fn(m_original, usb_ctx, stop);
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
        using ref_fn_t   = auto (*)(void *) -> void;
        using unref_fn_t = auto (*)(void *) -> void;
        using stop_fn_t  = auto (*)(void *) -> void;

        start_fn_t start_fn{nullptr};
        ref_fn_t ref_fn{nullptr};
        unref_fn_t unref_fn{nullptr};
        stop_fn_t stop_fn{nullptr};
    };
    void *m_original{nullptr};
    vtable m_vtable;
};

static_assert(detail::EventHandler<event_handler_ref>, "Not a proper USB event handler");

} // namespace co_usb
