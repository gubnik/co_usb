#pragma once

#include <libusb.h>

namespace co_usb
{

/**
 * @ingroup hotplug
 *
 * @brief An aggregate struct to pass to functions requiring device information.
 *
 * @note Default values are set to LIBUSB_HOTPLUG_MATCH_ANY.
 */
struct device_triplet
{
    int vid = LIBUSB_HOTPLUG_MATCH_ANY;
    int pid = LIBUSB_HOTPLUG_MATCH_ANY;
    int dev_class = LIBUSB_HOTPLUG_MATCH_ANY;

    constexpr bool operator==(device_triplet const &other) const noexcept
    {
        return other.vid == this->vid && other.pid == this->pid &&
               other.dev_class == this->dev_class;
    }
};

struct triplet_comparator
{
    constexpr bool operator()(const device_triplet &lhs, const device_triplet &rhs) const
    {
        constexpr auto cmp = [] (int const lhs, int const rhs) -> bool
        {
            return (lhs == rhs) || lhs == LIBUSB_HOTPLUG_MATCH_ANY ||
                   rhs == LIBUSB_HOTPLUG_MATCH_ANY;
        };
        return cmp(lhs.vid, rhs.vid) && cmp(lhs.pid, rhs.pid) && cmp(lhs.dev_class, rhs.dev_class);
    }
};

inline auto triplet_from_descriptor (libusb_device_descriptor const &desc)
{
    return device_triplet{
        .vid = desc.idVendor, .pid = desc.idProduct, .dev_class = desc.bDeviceClass};
}

}; // namespace co_usb
