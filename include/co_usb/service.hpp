#pragma once

#include <boost/capy/ex/execution_context.hpp>
#include <libusb-1.0/libusb.h>
#include <optional>
#include <stop_token>
#include <thread>
#include <utility>

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
    using handler_fn_t = void (*)(libusb_context *, std::stop_token);
    handler_service(boost::capy::execution_context &ctx);

    /**
     * @brief Creates a default handler thread
     *
     * Cannot be put into ctor because this must be optional
     * and services cannot take additional ctor params.
     */
    template <std::invocable<libusb_context *, std::stop_token> HandlerFn>
    void start_thread (libusb_context *ctx, HandlerFn &&handler_fn)
    {
        m_handler_thread = std::jthread{
            [ctx = ctx, handler_fn = std::forward<HandlerFn>(handler_fn)] (std::stop_token st)
            { handler_fn(ctx, st); }};
    }

    /**
     * @brief default handler function for the service
     */
    static void default_handler(libusb_context *ctx, std::stop_token st);

    std::stop_source stop_source();

    ~handler_service() override;
    void shutdown() override;

  private:
    std::optional<std::jthread> m_handler_thread;
};

} // namespace co_usb::detail
