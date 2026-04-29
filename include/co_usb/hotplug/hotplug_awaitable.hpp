#pragma once

#include "boost/capy/concept/io_awaitable.hpp"
#include "boost/capy/continuation.hpp"
#include "boost/capy/ex/io_env.hpp"
#include "co_usb/error.hpp"
#include "co_usb/hotplug/device_ref.hpp"
#include <boost/capy/io_result.hpp>
#include <co_usb/hotplug/hotplug.hpp>

namespace co_usb
{

/**
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
    hotplug_awaitable(libusb_context *ctx, int events, int flags, int vid, int pid, int dev_class);

    /**
     * @return always false, a hotplug cannot complete instantly without a roundtrip
     */
    bool await_ready() noexcept;

    /**
     * @brief suspends and registers the callback
     *
     * @param h @ref std::coroutine_handle to the awaiting coroutine
     * @param env @ref boost::capy::io_env* as per @ref boost::capy::IoAwaitable concept
     *
     * @return @ref std::noop_coroutine on success
     * @return @p h on submission error or on cancellation
     */
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h,
                                          boost::capy::io_env const *env);

    boost::capy::io_result<hotplug_event, device_ref> await_resume();

  private:
    libusb_context *m_ctx;
    int m_events;
    int m_flags;
    int m_vid;
    int m_pid;
    int m_dev_class;

    libusb_hotplug_callback_handle m_handle;

    usb_error m_error;
    struct cb_data
    {
        boost::capy::io_env const *io_env = nullptr;
        boost::capy::continuation cont;
        hotplug_event event;
        device_ref dev;
    } m_data;
};

static_assert(boost::capy::IoAwaitable<hotplug_awaitable>, "Not an IoAwaitable");

} // namespace co_usb
