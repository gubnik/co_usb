#pragma once

#include <libusb-1.0/libusb.h>
namespace co_usb
{

enum class hotplug_event
{
    arrived = LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
    left    = LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
};

enum class hotplug_flag
{
    enumerate = LIBUSB_HOTPLUG_ENUMERATE,
};

} // namespace co_usb
