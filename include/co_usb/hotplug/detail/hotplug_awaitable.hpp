#pragma once

#include "boost/capy/concept/io_awaitable.hpp"
#include "boost/capy/continuation.hpp"
#include "boost/capy/ex/io_env.hpp"
#include "co_usb/hotplug/flags.hpp"
#include "co_usb/usb_error.hpp"
#include "co_usb/wrapper/device_ref.hpp"
#include <boost/capy/io_result.hpp>
#include <system_error>

namespace co_usb::hotplug::detail
{

/**
 * @ingroup hotplug
 *
 * @brief Low-level awaitable primitive for strapping hotplug callbacks to a coroutine ecosystem.
 *
 * @details This is the lowest possible layer of abstraction over libusb's hotplug API. It should be
 * used for maximum flexibility or when the behaviour of @ref device_acceptor is unacceptable, e.g.
 * when no heap allocation is acceptable.
 *
 * Objects of this type are copyable since an awaitable object itself does not own any resource and
 * operates on handles, pointers and integers instead. It it safe to copy and to move in any state.
 *
 * @note Does not allocate.
 */
struct hotplug_awaitable
{
    struct op_result
    {
        std::error_code ec;
        event event;
        device_ref dev;
    };

    struct resumption_t
    {
        boost::capy::io_env const *io_env = nullptr;
        boost::capy::continuation cont;
        op_result *op_res;
    };

    hotplug_awaitable (libusb_context *ctx, libusb_hotplug_callback_handle *handle,
                       op_result *op_res, resumption_t *res, int events, int flags, int vid,
                       int pid, int dev_class)
        : ctx(ctx), handle(handle), op_res(op_res), res(res), events(events), flags(flags),
          vid(vid), pid(pid), dev_class(dev_class)
    {
    }

    /**
     * @return always false, a hotplug cannot complete instantly without a roundtrip
     */
    bool await_ready () noexcept
    {
        return false;
    }

    /**
     * @brief suspends and registers the callback
     *
     * @param h @ref std::coroutine_handle to the awaiting coroutine
     * @param env @ref boost::capy::io_env* as per @ref boost::capy::IoAwaitable concept
     *
     * @return @ref std::noop_coroutine on success
     * @return @p h on submission error or on cancellation
     */
    std::coroutine_handle<> await_suspend (std::coroutine_handle<> h,
                                           boost::capy::io_env const *env)
    {
        if (env->stop_token.stop_requested())
        {
            op_res->ec = std::make_error_code(std::errc::operation_canceled);
            return h;
        }
        res->cont = {h};
        res->io_env = env;
        res->op_res = op_res;
        auto r = libusb_hotplug_register_callback(
            ctx, events, flags, vid, pid, dev_class,
            [] (libusb_context *ctx, libusb_device *dev, libusb_hotplug_event ev,
                void *user_data) -> int
            {
                resumption_t *res = (resumption_t *)user_data;
                res->op_res->event = event(ev);
                res->op_res->dev = device_ref{dev};
                res->io_env->executor.post(res->cont);
                return 1;
            },
            res, handle);
        if (r != LIBUSB_SUCCESS)
        {
            op_res->ec = make_usb_error_code(static_cast<usb_error>(r));
            return h;
        }
        return std::noop_coroutine();
    }

    boost::capy::io_result<event, device_ref> await_resume ()
    {
        if (op_res->ec)
        {
            return {op_res->ec, {}, {}};
        }
        return {{}, op_res->event, op_res->dev};
    }

    libusb_context *ctx;
    libusb_hotplug_callback_handle *handle;
    op_result *op_res;
    resumption_t *res;
    int events;
    int flags;
    int vid;
    int pid;
    int dev_class;
};

static_assert(boost::capy::IoAwaitable<hotplug_awaitable>, "Not an IoAwaitable");

} // namespace co_usb::hotplug::detail
