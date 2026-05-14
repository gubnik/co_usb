#pragma once

#include "co_usb/device_ref.hpp"
#include "co_usb/device_triplet.hpp"
#include <boost/capy/io_result.hpp>
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

boost::capy::io_result<unique_dev_handle> open(libusb_context *ctx,
                                               device_triplet triplet) noexcept;
boost::capy::io_result<unique_dev_handle> open(libusb_device *dev) noexcept;
boost::capy::io_result<unique_dev_handle> open(device_ref dev) noexcept;

// Transfer
using transfer_deleter_t = std::decay_t<decltype(libusb_free_transfer)>;
using unique_transfer    = std::unique_ptr<libusb_transfer, transfer_deleter_t>;

} // namespace co_usb
