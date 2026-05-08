#pragma once

#include <libusb-1.0/libusb.h>
#include <memory>
#include <system_error>

namespace co_usb
{

/**
 * @brief Refcounted RAII guard for managing detachment of a kernel driver for an interface of a
 * device
 */
struct kernel_driver_guard
{
    kernel_driver_guard(libusb_device_handle *devh, int iface_num);
    kernel_driver_guard(std::error_code &errc, libusb_device_handle *devh, int iface_num) noexcept;

    libusb_device_handle *dev() const noexcept;
    int number() const noexcept;

    void release() noexcept;

  private:
    std::shared_ptr<libusb_device_handle> m_devh;
    int m_iface_num;
};

} // namespace co_usb
