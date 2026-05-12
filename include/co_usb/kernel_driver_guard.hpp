#pragma once

#include "co_usb/raii.hpp"
#include <boost/capy/io_result.hpp>
#include <libusb-1.0/libusb.h>
#include <memory>

namespace co_usb
{

/**
 * @brief Refcounted RAII guard for managing detachment of a kernel driver for an interface of a
 * device
 *
 * @note Acts as a shared_ptr
 */
struct kernel_driver_guard
{
    static boost::capy::io_result<kernel_driver_guard> detach(libusb_device_handle *devh,
                                                              int iface_num) noexcept;

    static boost::capy::io_result<kernel_driver_guard> detach(unique_dev_handle &devh,
                                                              int iface_num) noexcept;

    libusb_device_handle *dev() const noexcept;
    int number() const noexcept;

    /**
     * @brief Releases kernel guard and reattaches the driver
     */
    void release() noexcept;

  private:
    kernel_driver_guard(libusb_device_handle *devh, int iface_num);

    std::shared_ptr<libusb_device_handle> m_devh;
    int m_iface_num;
};

} // namespace co_usb
