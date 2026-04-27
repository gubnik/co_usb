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

struct hotplug_awaitable
{
    /**
     * @brief result of a hotplug operation
     */
    struct result
    {
        hotplug_event event;
        maybe_device_ref dev;
    };

    /**
     * @note I hate how this is like this is but this is how libusb works
     */
    hotplug_awaitable(libusb_context *ctx, int events, int flags, int vid, int pid, int dev_class);

    /**
     * @return always false, a hotplug cannot complete instantly without a roundtrip
     */
    bool await_ready() noexcept;

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h,
                                          boost::capy::io_env const *env);

    boost::capy::io_result<result> await_resume();

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
        result res;
    } m_data;
};

static_assert(boost::capy::IoAwaitable<hotplug_awaitable>, "Not an IoAwaitable");

} // namespace co_usb
