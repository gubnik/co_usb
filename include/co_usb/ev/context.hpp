#pragma once

#include "co_usb/ev/detail/handler_service.hpp"
#include <memory_resource>
namespace co_usb
{
namespace ev
{
struct context;
}
/**
 * @brief Factory for obtaining a co_usb context.
 */
template <ev::detail::EventHandler HandlerTy, typename... Args>
inline auto make_context(boost::capy::executor_ref exec, Args &&...args,
                         std::pmr::memory_resource *memres = std::pmr::get_default_resource())
    -> ev::context;
} // namespace co_usb
namespace co_usb::ev
{

/**
 * @brief A co_usb context referencing event handler service.
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
    template <detail::EventHandler HandlerTy, typename... Args>
    friend inline auto ::co_usb::make_context(boost::capy::executor_ref exec, Args &&...args,
                                              std::pmr::memory_resource *memres) -> context;

    explicit context (detail::handler_service *srv_ptr) noexcept : m_srv_ptr(srv_ptr)
    {
    }

    detail::handler_service *m_srv_ptr;
};

} // namespace co_usb::ev

namespace co_usb
{

template <ev::detail::EventHandler HandlerTy, typename... Args>
inline auto make_context (boost::capy::executor_ref exec, Args &&...args,
                          std::pmr::memory_resource *memres) -> ev::context
{
    boost::capy::execution_context &exec_ctx = exec.context();

    auto &srv = exec_ctx.use_service<ev::detail::handler_service>();
    srv.emplace_handler<HandlerTy>(std::forward<Args>(args)..., memres);
    srv.start();
    return ev::context{&srv};
}

} // namespace co_usb
