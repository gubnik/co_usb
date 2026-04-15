#pragma once

#include <libusb-1.0/libusb.h>
namespace co_usb
{

struct unique_handle
{
    unique_handle () noexcept : m_transfer(nullptr)
    {
    }

  private:
    libusb_transfer *m_transfer;
};

} // namespace co_usb
