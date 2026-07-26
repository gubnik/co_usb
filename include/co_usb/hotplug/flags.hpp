/**
 * @file Strongly typed wrappers libusb hotplug flag types (flags & events).
 */

#pragma once

#include "co_usb/detail/flag_type.hpp"
#include <libusb.h>

namespace co_usb
{

/**
 * @ingroup Hotplug
 *
 * @brief enum type for libusb_hotplug_event
 */
using hotplug_event = detail::flag_type<libusb_hotplug_event, struct hotplug_event_t>;

/**
 * @ingroup Hotplug
 *
 * @brief enum type for libusb_hotplug_flag
 */
using hotplug_flag = detail::flag_type<libusb_hotplug_flag, struct hotplug_flag_t>;

namespace hotplug_events
{
constexpr hotplug_event arrived{LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED};
constexpr hotplug_event left{LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT};
} // namespace hotplug_events

namespace hotplug_flags
{
constexpr hotplug_flag none{LIBUSB_HOTPLUG_NO_FLAGS};
constexpr hotplug_flag enumerate{LIBUSB_HOTPLUG_ENUMERATE};
}; // namespace hotplug_flags

} // namespace co_usb
