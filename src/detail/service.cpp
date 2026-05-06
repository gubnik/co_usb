#include <co_usb/detail/service.hpp>
#include <libusb-1.0/libusb.h>
#include <stdexcept>

co_usb::detail::handler_service::handler_service (boost::capy::execution_context &ctx)
    : boost::capy::execution_context::service()

{
}

std::stop_source co_usb::detail::handler_service::stop_source ()
{
    if (!m_handler_thread)
    {
        throw std::runtime_error{"Cannot get stop source of an unitialized service!"};
    }
    return m_handler_thread->get_stop_source();
}

void co_usb::detail::handler_service::default_handler (libusb_context *ctx, std::stop_token st)
{
    timeval tv = {.tv_sec = 0, .tv_usec = 10'000};
    for (;;)
    {
        auto r = libusb_handle_events_timeout(ctx, &tv);

        // stop was requested and no events are left to handle or can't be handled: exit!!
        if (r != LIBUSB_SUCCESS && st.stop_requested())
        {
            break;
        }
    }
}

co_usb::detail::handler_service::~handler_service ()
{
}

void co_usb::detail::handler_service::shutdown ()
{
    if (m_handler_thread)
    {
        m_handler_thread->join();
    }
}
