#pragma once

#include "co_usb/ev/detail/handler_service.hpp"
#include "co_usb/hotplug/detail/hotplug_awaitable.hpp"
#include "co_usb/hotplug/device_triplet.hpp"
#include "co_usb/hotplug/flags.hpp"
#include "co_usb/wrapper/device_ref.hpp"
#include <boost/capy/ex/this_coro.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/io_task.hpp>
#include <libusb.h>
#include <stop_token>

namespace co_usb
{

/**
 * @ingroup hotplug
 *
 * @brief Oneshot coroutine to get a one-time hotplug accept.
 *
 * @note Includes cancellation semantics.
 *
 * @details It may be more beneficial than @ref device_accepto for simpler applications
 * due to noticably less performance overhead and ease-of-use.
 */
inline auto oneshot_hotplug (hotplug_event events, hotplug_flag flags, device_triplet triplet)
    -> boost::capy::io_task<hotplug_event, device_ref>
{
    auto exec = co_await boost::capy::this_coro::executor;
    auto stop = co_await boost::capy::this_coro::stop_token;
    detail::hotplug_awaitable::op_result op_res{};
    detail::hotplug_awaitable::resumption_t res{};
    auto &srv = ::co_usb::ev::detail::get_handler_service(exec);
    auto ev_ref = srv.handler();
    libusb_context *ctx = srv.usb_context();
    libusb_hotplug_callback_handle handle{0};
    std::stop_callback stop_cb{stop, [&] ()
                               {
                                   if (handle == 0)
                                       return;
                                   libusb_hotplug_deregister_callback(ctx, handle);
                                   op_res.ec = std::make_error_code(std::errc::operation_canceled);
                                   res.io_env->executor.post(res.cont);
                               }};
    co_return co_await detail::hotplug_awaitable(ctx, &handle, &op_res, &res, events.bits,
                                                 flags.bits, triplet.vid, triplet.pid,
                                                 triplet.dev_class);
}

} // namespace co_usb
