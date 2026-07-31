#pragma once

#include <libusb.h>

namespace co_usb
{

/**
 * @ingroup wrapper
 *
 * @brief Wrapper for nullable libusb_device that increments ref count on ctor and decrements
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
    device_ref () noexcept : m_dev(nullptr)
    {
    }

    /**
     * @brief constructs a new ref object and increments ref count of underlying device
     */
    explicit device_ref (libusb_device *dev) noexcept : m_dev(dev)
    {
        libusb_ref_device(m_dev);
    }

    ~device_ref () noexcept
    {
        if (m_dev)
        {
            libusb_unref_device(m_dev);
        }
    }

    device_ref (device_ref const &other) noexcept : m_dev(other.m_dev)
    {
        if (m_dev)
            libusb_ref_device(m_dev);
    }

    device_ref &operator=(device_ref const &other) noexcept
    {
        m_dev = other.m_dev;
        if (m_dev)
            libusb_ref_device(m_dev);
        return *this;
    }

    /**
     * @brief Checks if a device reference is valid.
     *
     * @returns `true` if reference is valid.
     */
    auto valid () const noexcept -> bool
    {
        return m_dev != nullptr;
    }

    /**
     * @returns underlying pointer to @ref libusb_device
     */
    auto get () const noexcept -> libusb_device *
    {
        return m_dev;
    }

    /**
     * @brief Releases the device from a ref.
     *
     * @note Does not decrement ref counter on release to maintain device
     * lifetime in ambigious cases.
     *
     * @returns underlying pointer to @ref libusb_device
     */
    auto release () noexcept -> libusb_device *
    {
        auto tmp = m_dev;
        m_dev = nullptr;
        return tmp;
    }

  private:
    libusb_device *m_dev;
};

} // namespace co_usb
