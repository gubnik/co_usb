#include <co_usb/service.hpp>

co_usb::detail::handler_service::handler_service (boost::capy::execution_context &ctx)
    : boost::capy::execution_context::service()

{
}

/**
 * @brief Creates a default handler thread
 *
 * Cannot be put into ctor because this must be optional
 * and services cannot take additional ctor params.
 */
void co_usb::detail::handler_service::start_thread (libusb_context *ctx, std::stop_token st,
                                                    handler_fn_t handler_fn)
{
    m_handler_thread = std::thread{[=] () { handler_fn(ctx, st); }};
}

void co_usb::detail::handler_service::default_handler (libusb_context *ctx, std::stop_token st)
{
    timeval tv = {.tv_sec = 0, .tv_usec = 10'000};
    while (!st.stop_requested())
    {
        libusb_handle_events_timeout(ctx, &tv);
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
