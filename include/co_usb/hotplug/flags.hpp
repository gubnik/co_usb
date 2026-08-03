/**
 * @file flags.hpp
 * @brief Strongly typed wrappers libusb hotplug flag types (flags & events).
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
using event_type = detail::flag_type<libusb_hotplug_event, struct hotplug_event_t>;

/**
 * @ingroup hotplug
 *
 * @brief enum type for libusb_hotplug_flag
 */
using flag_type = detail::flag_type<libusb_hotplug_flag, struct hotplug_flag_t>;

/**
 * @ingroup hotplug
 *
 * @brief Concrete values of hotplug events.
 */
namespace events
{
constexpr event_type arrived{LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED};
constexpr event_type left{LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT};
} // namespace events

/**
 * @ingroup hotplug
 *
 * @brief Concrete values of hotplug flags.
 */
namespace flags
{
constexpr flag_type none{LIBUSB_HOTPLUG_NO_FLAGS};
constexpr flag_type enumerate{LIBUSB_HOTPLUG_ENUMERATE};
}; // namespace flags

} // namespace co_usb::hotplug
