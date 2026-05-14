#pragma once

#include <libusb.h>

namespace co_usb
{

/**
 * @brief Wrapper for nullable @ref libusb_device that increments ref count on ctor and decrements
 * on dtor.
 *
 * @details Objects of this type must be used for returning devices from hotplug callbacks since
 * normally devices obtained through this callback will expire unless their ref count was
 * incremented.
 */
struct device_ref
{
    /**
     * @brief constructs a null ref
     */
    device_ref() noexcept;

    /**
     * @brief constructs a new ref object and increments ref count of underlying device
     */
    explicit device_ref(libusb_device *dev) noexcept;

    ~device_ref() noexcept;

    device_ref(const device_ref &) noexcept;
    device_ref &operator=(const device_ref &) noexcept;

    /**
     * @returns underlying pointer to @ref libusb_device
     */
    libusb_device *get() const noexcept;

  private:
    libusb_device *m_dev;
};

} // namespace co_usb
