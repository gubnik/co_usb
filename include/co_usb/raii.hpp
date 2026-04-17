#pragma once

#include <libusb-1.0/libusb.h>
#include <memory>
#include <type_traits>

namespace co_usb::raii
{

// Context wrapper is omitted from there since libusb_init requires pointer to pointer which you
// cannot get from unique_ptr.
// @see context.hpp

// Device handle
using device_handle_deleter_t = std::decay_t<decltype(libusb_close)>;
using unique_dev_handle       = std::unique_ptr<libusb_device_handle, device_handle_deleter_t>;

// Transfer
using transfer_deleter_t = std::decay_t<decltype(libusb_free_transfer)>;
using unique_transfer    = std::unique_ptr<libusb_transfer, transfer_deleter_t>;

} // namespace co_usb::raii
