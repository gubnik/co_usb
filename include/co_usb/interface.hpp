#pragma once

#include <boost/capy/io_result.hpp>
#include <libusb-1.0/libusb.h>

namespace co_usb
{

struct interface
{
    interface(libusb_device_handle *devh, int iface);
    interface(std::error_code &errc, libusb_device_handle *devh, int iface) noexcept;

    ~interface();

    interface(const interface &other)            = delete;
    interface &operator=(const interface &other) = delete;

    interface(interface &&other);
    interface &operator=(interface &&other);

    libusb_device_handle *dev() const noexcept;
    int number() const noexcept;

    /**
     * @brief Releases the interface and attaches the driver if was detached
     */
    void release() const noexcept;

  private:
    libusb_device_handle *m_devh;
    int m_iface_num;
};

} // namespace co_usb
