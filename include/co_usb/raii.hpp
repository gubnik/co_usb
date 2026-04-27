#pragma once

#include "co_usb/hotplug/device_ref.hpp"
#include <libusb-1.0/libusb.h>
#include <memory>
#include <type_traits>

namespace co_usb
{

// Context wrapper is omitted from there since libusb_init requires pointer to pointer which you
// cannot get from unique_ptr.
// @see context.hpp

// Device handle
using dev_handle_deleter_t = std::decay_t<decltype(libusb_close)>;
using unique_dev_handle    = std::unique_ptr<libusb_device_handle, dev_handle_deleter_t>;

unique_dev_handle open_vid_pid(libusb_context *, uint16_t vid, uint16_t pid);
unique_dev_handle open(libusb_device *dev);
unique_dev_handle open(device_ref dev);

// Transfer
using transfer_deleter_t = std::decay_t<decltype(libusb_free_transfer)>;
using unique_transfer    = std::unique_ptr<libusb_transfer, transfer_deleter_t>;

} // namespace co_usb
