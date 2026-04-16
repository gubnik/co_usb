#include <co_usb/handler.hpp>
#include <libusb-1.0/libusb.h>
#include <stop_token>

void co_usb::simple_handler (co_usb::unique_context ctx, std::stop_token st, timeval tv)
{
    while (!st.stop_requested())
    {
        libusb_handle_events_timeout(ctx.get(), &tv);
    }
}
