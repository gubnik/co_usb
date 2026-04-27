#pragma once

#include <libusb-1.0/libusb.h>
#include <optional>
namespace co_usb
{

/**
 * @brief Wrapper for nullable @ref libusb_device that increments ref count on ctor and decrements
 * on dtor
 */
struct device_ref
{
    /**
     * @brief constructs a null ref
     */
    device_ref() noexcept;

    explicit device_ref(libusb_device *dev) noexcept;

    ~device_ref() noexcept;

    device_ref(const device_ref &) noexcept;
    device_ref &operator=(const device_ref &) noexcept;

    /**
     * @returns underlying pointer to @ref libusb_device
     *
     * @throws @ref std::runtime_error if internal device is null
     */
    libusb_device *get() const;

    /**
     * @returns optional of an underlying pointer to @ref libusb_device
     *
     * @note does not throw
     */
    std::optional<libusb_device *> get_opt() const noexcept;

  private:
    libusb_device *m_dev;
};

} // namespace co_usb
