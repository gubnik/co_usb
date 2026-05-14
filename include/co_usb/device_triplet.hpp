#pragma once

#include <libusb.h>

namespace co_usb
{

/**
 * @brief Aggregate struct to pass to functions requiring device information.
 *
 * Default values are set to LIBUSB_HOTPLUG_MATCH_ANY for brievity in hotplug usage.
 */
struct device_triplet
{
    int vid       = LIBUSB_HOTPLUG_MATCH_ANY;
    int pid       = LIBUSB_HOTPLUG_MATCH_ANY;
    int dev_class = LIBUSB_HOTPLUG_MATCH_ANY;
};

} // namespace co_usb
