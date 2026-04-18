#pragma once

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

// Transfer
using transfer_deleter_t = std::decay_t<decltype(libusb_free_transfer)>;
using unique_transfer    = std::unique_ptr<libusb_transfer, transfer_deleter_t>;

// Interface
struct interface_holder
{
    explicit interface_holder(libusb_device_handle *devh, int interface_num);
    ~interface_holder() noexcept;

    interface_holder(interface_holder const &)            = delete;
    interface_holder(interface_holder &&)                 = delete;
    interface_holder &operator=(interface_holder const &) = delete;
    interface_holder &operator=(interface_holder &&)      = delete;

  private:
    libusb_device_handle *m_devh;
    int m_interface_num;
};

} // namespace co_usb
