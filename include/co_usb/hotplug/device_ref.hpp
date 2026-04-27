#pragma once

#include <libusb-1.0/libusb.h>
#include <optional>
namespace co_usb
{

/**
 * @brief Wrapper for non-null @ref libusb_device that increments ref count on ctor and decrements
 * on dtor
 */
struct device_ref
{
    explicit device_ref(libusb_device *dev);

    ~device_ref() noexcept;

    device_ref(const device_ref &) noexcept;
    device_ref &operator=(const device_ref &) noexcept;

    libusb_device *raw() const noexcept;

  private:
    libusb_device *m_dev;
};

using maybe_device_ref = std::optional<device_ref>;

} // namespace co_usb
