#pragma once

#include <boost/capy/ex/execution_context.hpp>
#include <libusb-1.0/libusb.h>
#include <optional>
#include <stop_token>
#include <thread>

namespace co_usb::detail
{

/**
 * @brief Service for handling libusb events
 *
 * @details creates a primitive libusb event handler thread. Said handler is guaranteed to NOT
 * support multithreaded handling and must be assumed to be as primitive as humanly possible;
 *
 * @note 100% OPTIONAL
 */
struct handler_service : public boost::capy::execution_context::service
{
    handler_service(boost::capy::execution_context &ctx);

    /**
     * @brief Creates a default handler thread
     *
     * Cannot be put into ctor because this must be optional
     * and services cannot take additional ctor params.
     */
    void start_thread(libusb_context *ctx, std::stop_token st,
                      timeval tv = {.tv_sec = 0, .tv_usec = 10'000});

    ~handler_service() override;
    void shutdown() override;

  private:
    std::optional<std::jthread> m_handler_thread;
};

} // namespace co_usb::detail
