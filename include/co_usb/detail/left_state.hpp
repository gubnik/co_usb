#pragma once

#include <atomic>
#include <libusb-1.0/libusb.h>

namespace co_usb::detail
{

struct left_state
{
    std::atomic_flag flag;
    libusb_context *ctx;
    libusb_hotplug_callback_handle handle;
};

} // namespace co_usb::detail
