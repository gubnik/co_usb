#pragma once

#include "co_usb/raii.hpp"
#include <boost/capy/io_result.hpp>
#include <libusb-1.0/libusb.h>

namespace co_usb
{

struct interface;

struct interface
{
    static boost::capy::io_result<interface> claim(libusb_device_handle *, int) noexcept;
    static boost::capy::io_result<interface> claim(unique_dev_handle &, int) noexcept;

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
    interface(libusb_device_handle *devh, int iface) noexcept;

    libusb_device_handle *m_devh;
    int m_iface_num;
};

} // namespace co_usb
