#include "co_usb/error.hpp"
#include "co_usb/hotplug/device_ref.hpp"
#include <co_usb/hotplug/hotplug_awaitable.hpp>
#include <coroutine>
#include <libusb-1.0/libusb.h>

co_usb::hotplug_awaitable::hotplug_awaitable (libusb_context *ctx, int events, int flags, int vid,
                                              int pid, int dev_class)
    : m_ctx(ctx), m_events(events), m_flags(flags), m_vid(vid), m_pid(pid), m_dev_class(dev_class)
{
}

bool co_usb::hotplug_awaitable::await_ready () noexcept
{
    return false;
}

std::coroutine_handle<> co_usb::hotplug_awaitable::await_suspend (std::coroutine_handle<> h,
                                                                  boost::capy::io_env const *env)
{
    if (env->stop_token.stop_requested())
    {
        m_data.res.event = co_usb::hotplug_event::left;
        m_data.res.dev   = device_ref{nullptr};
        m_error          = usb_error::unknown;
        return h;
    }
    m_data.cont   = {h};
    m_data.io_env = env;
    auto r        = libusb_hotplug_register_callback(
        m_ctx, m_events, m_flags, m_vid, m_pid, m_dev_class,
        [] (libusb_context *ctx, libusb_device *dev, libusb_hotplug_event ev,
            void *user_data) -> int
        {
            auto *data      = (cb_data *)user_data;
            data->res.event = (ev == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
                                  ? co_usb::hotplug_event::arrived
                                  : co_usb::hotplug_event::left;
            data->res.dev   = device_ref{dev};
            data->io_env->executor.post(data->cont);
            return 0;
        },
        &m_data, &m_handle);
    if (r != LIBUSB_SUCCESS)
    {
        m_error = static_cast<usb_error>(r);
        return h;
    }
    return std::noop_coroutine();
}

boost::capy::io_result<co_usb::hotplug_event, co_usb::device_ref>
co_usb::hotplug_awaitable::await_resume ()
{
    libusb_hotplug_deregister_callback(m_ctx, m_handle);
    return {make_usb_error_code(m_error), m_data.res.event, m_data.res.dev};
}
