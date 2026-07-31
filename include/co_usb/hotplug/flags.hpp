/**
 * @file Strongly typed wrappers libusb hotplug flag types (flags & events).
 */

#pragma once

#include "co_usb/detail/flag_type.hpp"
#include <libusb.h>

namespace co_usb::hotplug
{

/**
 * @ingroup hotplug
 *
 * @brief enum type for libusb_hotplug_event
 */
using event = detail::flag_type<libusb_hotplug_event, struct hotplug_event_t>;

/**
 * @ingroup hotplug
 *
 * @brief enum type for libusb_hotplug_flag
 */
using flag = detail::flag_type<libusb_hotplug_flag, struct hotplug_flag_t>;

namespace events
{
constexpr event arrived{LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED};
constexpr event left{LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT};
} // namespace events

namespace flags
{
constexpr flag none{LIBUSB_HOTPLUG_NO_FLAGS};
constexpr flag enumerate{LIBUSB_HOTPLUG_ENUMERATE};
}; // namespace flags

} // namespace co_usb::hotplug
