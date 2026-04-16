#pragma once

#include "co_usb/unique_context.hpp"
#include <libusb-1.0/libusb.h>
#include <stop_token>

namespace co_usb
{

void simple_handler(unique_context, std::stop_token, timeval = {.tv_sec = 0, .tv_usec = 10'000});
void multithreaded_handler(unique_context);

} // namespace co_usb
