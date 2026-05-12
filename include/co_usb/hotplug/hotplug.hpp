#pragma once

#include <libusb-1.0/libusb.h>
namespace co_usb
{

/**
 * @brief enum class wrapper for hotplug events
 *
 * Used in @ref hotplug_awaitable
 */
enum class hotplug_event
{
    arrived = LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
    left    = LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
};

/**
 * @brief wrapper for hotplug flags
 *
 * There is only two of them
 */
enum class hotplug_flag
{
    none      = LIBUSB_HOTPLUG_NO_FLAGS,
    enumerate = LIBUSB_HOTPLUG_ENUMERATE,
};

} // namespace co_usb
