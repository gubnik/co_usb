#include "co_usb/error.hpp"
#include <atomic>
#include <co_usb/hotplug/device_left_signal.hpp>
#include <libusb-1.0/libusb.h>
#include <memory>
#include <system_error>

struct co_usb::cb_data
{
    libusb_context *ctx;
    libusb_hotplug_callback_handle handle;
    std::atomic_flag flag{false};
};

co_usb::device_left_signal::device_left_signal (libusb_context *ctx, int vid, int pid,
                                                int dev_class)
{
    m_data      = std::make_unique<cb_data>();
    m_data->ctx = ctx;
    auto r      = libusb_hotplug_register_callback(
        m_data->ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0 /* don't enumerate */, vid, pid, dev_class,
        [] (libusb_context *ctx, libusb_device *dev, libusb_hotplug_event ev,
            void *user_data) -> int
        {
            auto *data = (cb_data *)user_data;
            data->flag.test_and_set();
            libusb_hotplug_deregister_callback(data->ctx, data->handle);
            return 0;
        },
        m_data.get(), &m_data->handle);
    if (r != LIBUSB_SUCCESS)
    {
        throw std::system_error{make_usb_error_code(static_cast<usb_error>(r))};
    }
}

co_usb::device_left_signal::~device_left_signal () noexcept
{
    if (m_data)
        libusb_hotplug_deregister_callback(m_data->ctx, m_data->handle);
}

co_usb::device_left_signal::device_left_signal (device_left_signal &&other)
{
    if (m_data)
        libusb_hotplug_deregister_callback(m_data->ctx, m_data->handle);
    m_data = std::move(other.m_data);
}

co_usb::device_left_signal &co_usb::device_left_signal::operator=(device_left_signal &&other)
{
    if (m_data)
        libusb_hotplug_deregister_callback(m_data->ctx, m_data->handle);
    m_data = std::move(other.m_data);
    return *this;
}

bool co_usb::device_left_signal::device_left () const
{
    if (!m_data)
        return true;
    return m_data->flag.test();
}
