#pragma once

#include "boost/capy/ex/io_env.hpp"
#include <atomic>
#include <coroutine>
#include <libusb-1.0/libusb.h>

namespace co_usb
{

struct hotplug_state
{
    std::atomic<libusb_hotplug_event> signal;
};

struct hotplug_res
{
    libusb_hotplug_event ev;
    libusb_device *dev;
};

struct hotplug_event
{
    bool await_ready ()
    {
        return false;
    }

    std::coroutine_handle<> await_suspend (std::coroutine_handle<>,
                                           boost::capy::io_env const *env)
    {
        return std::noop_coroutine();
    }

    hotplug_res await_resume ()
    {
        return {};
    }

    hotplug_state state;
};

} // namespace co_usb
