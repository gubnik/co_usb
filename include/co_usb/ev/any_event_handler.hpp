/**
 * @file any_event_handler.hpp
 * @brief Owning type-erased wrapper for @ref EventHandler.
 */

#pragma once

#include "co_usb/ev/detail/event_handler.hpp"
#include <concepts>
#include <memory_resource>
#include <type_traits>
#include <utility>

namespace co_usb::ev
{

struct event_handler_ref;

/**
 * @ingroup event_handler
 *
 * @brief Owning type-erased wrapper for @ref EventHandler.
 *
 * @details This type is used to uniformly store objects of any type that conform to
 * @ref co_usb::ev::detail::EventHandler concept. It allocates the space for the buffer using a
 * given pointer to a memory resource and will preserve this pointer inside of itself.
 *
 * @see event_handler_ref
 */
struct any_event_handler
{
    friend event_handler_ref;

    explicit any_event_handler () noexcept
    {
    }

    template <detail::EventHandler HandlerTy, typename... Args>
        requires(!std::same_as<HandlerTy, any_event_handler> &&
                 !std::same_as<HandlerTy, event_handler_ref> &&
                 std::constructible_from<HandlerTy, Args...>)
    explicit any_event_handler(std::pmr::memory_resource *memres, Args &&...args)
        : m_vtable(make_vtable<HandlerTy>()), m_memres(memres),
          m_storage(construct_storage<HandlerTy>(m_memres, std::forward<Args>(args)...))
    {
    }

    template <detail::EventHandler HandlerTy, typename... Args>
        requires(!std::same_as<HandlerTy, any_event_handler> &&
                 !std::same_as<HandlerTy, event_handler_ref> &&
                 std::constructible_from<HandlerTy, Args...>)
    explicit any_event_handler(Args &&...args)
        : m_vtable(make_vtable<HandlerTy>()), m_memres(std::pmr::get_default_resource()),
          m_storage(construct_storage<HandlerTy>(m_memres, std::forward<Args>(args)...))
    {
    }

    template <detail::EventHandler HandlerTy, typename... Args>
        requires(!std::same_as<HandlerTy, any_event_handler> &&
                 !std::same_as<HandlerTy, event_handler_ref> &&
                 std::constructible_from<HandlerTy, Args...>)
    auto emplace (std::pmr::memory_resource *memres, Args &&...args)
    {
        destroy();
        m_memres = memres;
        m_storage = construct_storage<HandlerTy>(m_memres, std::forward<Args>(args)...);
        m_vtable = make_vtable<HandlerTy>();
    }

    template <detail::EventHandler HandlerTy, typename... Args>
        requires(!std::same_as<HandlerTy, any_event_handler> &&
                 !std::same_as<HandlerTy, event_handler_ref> &&
                 std::constructible_from<HandlerTy, Args...>)
    auto emplace (Args &&...args)
    {
        emplace<HandlerTy>(m_memres, std::forward<Args>(args)...);
    }

    any_event_handler(any_event_handler const &other) = delete;
    any_event_handler &operator=(any_event_handler const &other) = delete;

    any_event_handler (any_event_handler &&other) noexcept
        : m_vtable(other.m_vtable), m_memres(other.m_memres), m_storage(other.m_storage)
    {
        other.m_storage = nullptr;
    }

    any_event_handler &operator=(any_event_handler &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        destroy();
        m_vtable = other.m_vtable;
        m_memres = other.m_memres;
        m_storage = other.m_storage;
        other.m_storage = nullptr;
        return *this;
    }

    ~any_event_handler ()
    {
        destroy();
    }

    auto valid () const noexcept -> bool
    {
        return m_storage;
    }

    auto start (libusb_context *usb_ctx, std::stop_token stop) -> bool
    {
        if (!m_storage)
        {
            return false;
        }
        return m_vtable.start_fn(m_storage, usb_ctx, std::move(stop));
    }

    auto ref () noexcept -> void
    {
        if (!m_storage)
        {
            return;
        }
        return m_vtable.ref_fn(m_storage);
    }

    auto unref () noexcept -> void
    {
        if (!m_storage)
        {
            return;
        }
        return m_vtable.unref_fn(m_storage);
    }

    auto stop () -> void
    {
        if (!m_storage)
        {
            return;
        }
        return m_vtable.stop_fn(m_storage);
    }

  private:
    struct vtable
    {
        using start_fn_t = auto (*)(void *, libusb_context *, std::stop_token) -> bool;
        using ref_fn_t = auto (*)(void *) noexcept -> void;
        using unref_fn_t = auto (*)(void *) noexcept -> void;
        using stop_fn_t = auto (*)(void *) -> void;
        using dtor_fn_t = auto (*)(void *) -> void;

        start_fn_t start_fn{nullptr};
        ref_fn_t ref_fn{nullptr};
        unref_fn_t unref_fn{nullptr};
        stop_fn_t stop_fn{nullptr};
        dtor_fn_t dtor_fn{nullptr};
        size_t size;
        size_t align;
    };

    template <detail::EventHandler HandlerTy, typename... Args>
    auto construct_storage (std::pmr::memory_resource *memres, Args &&...args) -> void *
    {
        using handler_t = std::remove_cvref_t<HandlerTy>;
        const auto size = sizeof(handler_t);
        const auto align = alignof(handler_t);
        void *const ptr = memres->allocate(size, align);
        try
        {
            new (ptr) handler_t(std::forward<Args>(args)...);
        }
        catch (...)
        {
            memres->deallocate(ptr, size, align);
            throw;
        }
        return ptr;
    }

    template <detail::EventHandler HandlerTy> static auto make_vtable () -> vtable
    {
        return vtable{
            .start_fn = +[] (void *orig, libusb_context *ctx, std::stop_token stop) -> bool
            { return static_cast<HandlerTy *>(orig)->start(ctx, stop); },
            .ref_fn = +[] (void *orig) noexcept -> void
            { return static_cast<HandlerTy *>(orig)->ref(); },
            .unref_fn = +[] (void *orig) noexcept -> void
            { return static_cast<HandlerTy *>(orig)->unref(); },
            .stop_fn = +[] (void *orig) -> void { return static_cast<HandlerTy *>(orig)->stop(); },
            .dtor_fn = +[] (void *orig) -> void
            { return static_cast<HandlerTy *>(orig)->~HandlerTy(); },
            .size = sizeof(HandlerTy),
            .align = alignof(HandlerTy),
        };
    }

    auto destroy () -> void
    {
        if (!m_storage)
            return;
        m_vtable.dtor_fn(m_storage);
        m_memres->deallocate(m_storage, m_vtable.size, m_vtable.align);
        m_storage = nullptr;
    }

    vtable m_vtable;
    std::pmr::memory_resource *m_memres = std::pmr::get_default_resource();
    void *m_storage{nullptr};
};
static_assert(detail::EventHandler<any_event_handler>, "Not a proper USB event handler");

} // namespace co_usb::ev
