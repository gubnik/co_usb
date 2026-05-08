#include <co_usb/context.hpp>
#include <co_usb/error.hpp>

void co_usb::context<co_usb::use_service::no>::init ()
{
    auto r = libusb_init(&m_ctx);
    if (r != LIBUSB_SUCCESS)
    {
        throw std::runtime_error{"Cannot initialize libusb"};
    }
}

co_usb::context<co_usb::use_service::no>::context ()
{
    init();
}

co_usb::context<co_usb::use_service::no>::~context ()
{
    if (m_ctx)
        libusb_exit(m_ctx);
}

void co_usb::default_handler (libusb_context *ctx, std::stop_token st)
{
    const timeval ctv = {.tv_sec = 0, .tv_usec = 10'000};
    while (!st.stop_requested())
    {
        timeval tv = ctv;
        auto r     = libusb_handle_events_timeout(ctx, &tv);
        if (r != LIBUSB_SUCCESS)
        {
            throw std::system_error{make_usb_error_code(static_cast<usb_error>(r))};
        }
    }
}
