#include "co_usb/device_triplet.hpp"
#include "co_usb/error.hpp"
#include <atomic>
#include <co_usb/hotplug/left_signal.hpp>
#include <libusb-1.0/libusb.h>
#include <memory>
#include <memory_resource>
#include <system_error>

co_usb::left_signal::left_signal (libusb_context *ctx, co_usb::device_triplet triplet,
                                  std::pmr::memory_resource *memory_resource)
{
    auto allocator   = std::pmr::polymorphic_allocator<detail::left_state>(memory_resource);
    m_state_ptr      = std::allocate_shared<detail::left_state>(std::move(allocator));
    m_state_ptr->ctx = ctx;
    auto r           = libusb_hotplug_register_callback(
        m_state_ptr->ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, triplet.vid,
        triplet.pid, triplet.dev_class,
        [] (libusb_context *ctx, libusb_device *dev, libusb_hotplug_event ev,
            void *user_data) -> int
        {
            auto *data = (detail::left_state *)user_data;
            data->flag.test_and_set();
            libusb_hotplug_deregister_callback(data->ctx, data->handle);
            return 0;
        },
        m_state_ptr.get(), &m_state_ptr->handle);
    if (r != LIBUSB_SUCCESS)
    {
        throw std::system_error{make_usb_error_code(static_cast<usb_error>(r))};
    }
}

co_usb::left_signal::~left_signal () noexcept
{
    disarm();
}

co_usb::left_signal::left_signal (const left_signal &other)
{
    disarm();
    m_state_ptr = other.m_state_ptr;
}

co_usb::left_signal &co_usb::left_signal::operator=(const left_signal &other)
{
    disarm();
    m_state_ptr = other.m_state_ptr;
    return *this;
}

co_usb::left_signal::left_signal (left_signal &&other)
{
    disarm();
    m_state_ptr = std::move(other.m_state_ptr);
}

co_usb::left_signal &co_usb::left_signal::operator=(left_signal &&other)
{
    disarm();
    m_state_ptr = std::move(other.m_state_ptr);
    return *this;
}

bool co_usb::left_signal::device_left () const
{
    return m_state_ptr->flag.test();
}

bool co_usb::left_signal::disarm ()
{
    if (!m_state_ptr)
        return true;
    if (!m_state_ptr->flag.test())
        return true;
    libusb_hotplug_deregister_callback(m_state_ptr->ctx, m_state_ptr->handle);
    return true;
}

co_usb::left_token::left_token (co_usb::left_signal &signal) : m_state_ptr(signal.m_state_ptr)
{
}

bool co_usb::left_token::device_left () const
{
    return m_state_ptr->flag.test();
}
