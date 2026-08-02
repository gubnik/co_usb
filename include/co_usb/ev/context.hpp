/**
 * @file context.hpp
 * @brief Factory for obtaining a co_usb context.
 */

#pragma once

#include "co_usb/ev/detail/handler_service.hpp"
#include "co_usb/wrapper/detail/error_protocol.hpp"
#include "co_usb/wrapper/error_protocol.hpp"
#include <libusb.h>
#include <memory_resource>
#include <system_error>

namespace co_usb
{

namespace ev
{
struct context;
}

namespace detail
{
auto raw_context(::co_usb::ev::detail::handler_service *srv);

}

} // namespace co_usb

namespace co_usb::ev
{

/**
 * @brief A co_usb context referencing event handler service.
 *
 * @details This type acts as a handle to a service without exposing the service itself
 * to the user.
 *
 * @par The problem
 *
 * The reason for this type to exist is the fact that `libusb_context` can accept initialization
 * options, the user can and will want to assign an event handler and the process of initializing a
 * service turns into a long boilerplate. The `context` aims to smooth out this interaction by
 * providing a thin wrapper interface for initializing the service.
 *
 * @par The intrusivity and the lifetimes
 *
 * The `context` type is by design only a handle to a service, not in any way intrusive.
 * An alternative approach could see the context itself become a place for the event handler,
 * `libusb_context` and stop source to reside in. However, this would mean that the lifetime of this
 * data would be decoupled from the executor, and a slightly wrong code regarding `context` lifetime
 * would see the invalid memory access to a destroyed object occur. To prevent this scenario, the
 * data HAS to live in a service storage directly bound to an executor lifetime.
 *
 * @par Limitations
 *
 * Creating multiple contexts for a single executor is well-defined to be wrong. It will
 * reinitialize the context and rebind and restart the event handler, spawning multiple different
 * event handlers or services is unsupported within a single executor on a level of Capy. Creating
 * multiple single-threaded event handlers is unsupported by libusb and will break. If the goal was
 * to implement a multi-threaded event handling - provide a custom implementation of @ref
 * co_usb::ev::detail::EventHandler that properly manages the locking of libusb events.
 */
struct context
{
    auto get_token () const noexcept
    {
        return m_srv_ptr->get_token();
    }

    auto request_stop ()
    {
        m_srv_ptr->request_stop();
    }

    auto handler () -> event_handler_ref
    {
        return m_srv_ptr->handler();
    }

    auto usb_context () -> libusb_context *
    {
        return m_srv_ptr->usb_context();
    }

  private:
    friend auto ::co_usb::detail::raw_context(::co_usb::ev::detail::handler_service *srv);

    explicit context (detail::handler_service *srv_ptr) noexcept : m_srv_ptr(srv_ptr)
    {
    }

    detail::handler_service *m_srv_ptr;
};

} // namespace co_usb::ev

namespace co_usb
{

namespace detail
{

inline auto raw_context (::co_usb::ev::detail::handler_service *srv)
{
    return ev::context{srv};
}

} // namespace detail

template <ev::detail::EventHandler HandlerTy, ::co_usb::detail::ErrorProtocol<ev::context> ErrorTy,
          typename... Args>
inline auto make_context (boost::capy::executor_ref exec, std::pmr::memory_resource *memres,
                          ErrorTy &&errp, Args &&...args) -> ev::context
{
    boost::capy::execution_context &exec_ctx = exec.context();

    auto &srv = exec_ctx.use_service<ev::detail::handler_service>();
    std::error_code ec = srv.init_context();
    if (ec)
    {
        return errp.template with_error<ev::context>(ec);
    }
    srv.emplace_handler<HandlerTy>(std::forward<Args>(args)..., memres);
    srv.start();
    return errp.template with_success<ev::context>(detail::raw_context(&srv));
}

template <ev::detail::EventHandler HandlerTy, typename... Args>
inline auto make_context (boost::capy::executor_ref exec, std::pmr::memory_resource *memres,
                          Args &&...args) -> ev::context
{
    return make_context<HandlerTy, detail::as_exception_t, Args...>(exec, memres, as_exception(),
                                                                    std::forward<Args>(args)...);
}

template <ev::detail::EventHandler HandlerTy, ::co_usb::detail::ErrorProtocol<ev::context> ErrorTy,
          typename... Args>
inline auto make_context (boost::capy::executor_ref exec, ErrorTy &&errp, Args &&...args)
    -> ev::context
{
    return make_context<HandlerTy, ErrorTy, Args...>(exec, std::pmr::get_default_resource(),
                                                     std::forward<ErrorTy>(errp),
                                                     std::forward<Args>(args)...);
}

template <ev::detail::EventHandler HandlerTy, typename... Args>
inline auto make_context (boost::capy::executor_ref exec, Args &&...args) -> ev::context
{
    return make_context<HandlerTy, detail::as_exception_t, Args...>(
        exec, std::pmr::get_default_resource(), as_exception(), std::forward<Args>(args)...);
}

template <ev::detail::EventHandler HandlerTy, ::co_usb::detail::ErrorProtocol<ev::context> ErrorTy,
          typename... Args>
inline auto make_context_with_options (boost::capy::executor_ref exec,
                                       std::span<const libusb_init_option> options,
                                       std::pmr::memory_resource *memres, ErrorTy &&errp,
                                       Args &&...args) -> ev::context
{
    boost::capy::execution_context &exec_ctx = exec.context();

    auto &srv = exec_ctx.use_service<ev::detail::handler_service>();
    std::error_code ec = srv.init_context(options);
    if (ec)
    {
        return errp.template with_error<ev::context>(ec);
    }
    srv.emplace_handler<HandlerTy>(std::forward<Args>(args)..., memres);
    srv.start();
    return errp.template with_success<ev::context>(detail::raw_context(&srv));
}

template <ev::detail::EventHandler HandlerTy, typename... Args>
inline auto make_context_with_options (boost::capy::executor_ref exec,
                                       std::span<const libusb_init_option> options,
                                       std::pmr::memory_resource *memres, Args &&...args)
    -> ev::context
{
    return make_context_with_options<HandlerTy, detail::as_exception_t, Args...>(
        exec, options, memres, as_exception(), std::forward<Args>(args)...);
}

template <ev::detail::EventHandler HandlerTy, ::co_usb::detail::ErrorProtocol<ev::context> ErrorTy,
          typename... Args>
inline auto make_context_with_options (boost::capy::executor_ref exec,
                                       std::span<const libusb_init_option> options, ErrorTy &&errp,
                                       Args &&...args) -> ev::context
{
    return make_context_with_options<HandlerTy, ErrorTy, Args...>(
        exec, options, std::pmr::get_default_resource(), std::forward<ErrorTy>(errp),
        std::forward<Args>(args)...);
}

template <ev::detail::EventHandler HandlerTy, typename... Args>
inline auto make_context_with_options (boost::capy::executor_ref exec,
                                       std::span<const libusb_init_option> options, Args &&...args)
    -> ev::context
{
    return make_context_with_options<HandlerTy, detail::as_exception_t, Args...>(
        exec, options, std::pmr::get_default_resource(), as_exception(),
        std::forward<Args>(args)...);
}

} // namespace co_usb
