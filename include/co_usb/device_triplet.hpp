#pragma once

#include <libusb-1.0/libusb.h>

namespace co_usb
{

struct device_triplet
{
    int vid       = LIBUSB_HOTPLUG_MATCH_ANY;
    int pid       = LIBUSB_HOTPLUG_MATCH_ANY;
    int dev_class = LIBUSB_HOTPLUG_MATCH_ANY;
};

} // namespace co_usb
