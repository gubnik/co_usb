/**
 * @file handler_service.hpp
 * @brief Capy execution_context service wrapping a libusb event handler.
 */

#pragma once

#include "co_usb/ev/any_event_handler.hpp"
#include "co_usb/ev/detail/event_handler.hpp"
#include "co_usb/ev/event_handler_ref.hpp"
#include <boost/capy/ex/execution_context.hpp>
#include <boost/capy/ex/executor_ref.hpp>
#include <concepts>
#include <libusb.h>
#include <stdexcept>
#include <stop_token>
#include <utility>

namespace co_usb
{
struct context;
}

namespace co_usb::ev::detail
{

/**
 * @ingroup event_handler
 *
 * @brief Execution context service that owns a libusb context and an event handler.
 *
 * @details Since co_usb is bound to Capy and as such its task could get rather scattered across
 * executor threads, there must be a uniform way to access co_usb's `libusb_context` and the running
 * event handler. Thankfuly, Capy provides just the needed facilities in form of executor services.
 *
 * To create this service, refer to @ref co_usb::context
 *
 * @code
 * int main (int argc, char **argv)
 * {
 *     boost::capy::thread_pool;
 *     // this creates handler service with handler_type event handler
 *     co_usb::context usb_ctx = co_usb::make_context<handler_type>(...);
 * }
 * @endcode
 */
struct handler_service : public boost::capy::execution_context::service
{
    friend struct co_usb::context;

    /**
     * @brief Construct the handler service and initialize libusb.
     * @param exec_ctx Execution context to attach the service to.
     * @throws std::runtime_error if `libusb_init` fails.
     */
    explicit handler_service (boost::capy::execution_context &exec_ctx)
        : boost::capy::execution_context::service{}, m_handler{}
    {
        libusb_context *ctx;
        auto r = libusb_init(&ctx);
        if (r != LIBUSB_SUCCESS)
        {
            throw std::runtime_error{"Cannot initialize libusb"};
        }
        m_usb_ctx = ctx;
    }

    /**
     * @brief Destructor that stops the handler and releases the libusb context.
     *
     * Calls `shutdown()` and then `libusb_exit(m_usb_ctx)`.
     */
    ~handler_service () override
    {
        shutdown();
        libusb_exit(m_usb_ctx);
    }

    /**
     * @brief Emplace a concrete event handler and return a reference wrapper.
     *
     * This requires `HandlerTy` to satisfy `detail::EventHandler` and be neither
     * `any_event_handler` nor `event_handler_ref`.
     *
     * @tparam HandlerTy Concrete event handler type.
     * @tparam Args Constructor argument types for `HandlerTy`.
     * @param args Constructor arguments for the event handler.
     * @param memres Memory resource used by the internal handler storage.
     * @return `event_handler_ref` referencing the stored handler.
     */
    template <detail::EventHandler HandlerTy, typename... Args>
        requires(!std::same_as<HandlerTy, any_event_handler> &&
                 !std::same_as<HandlerTy, event_handler_ref> &&
                 std::constructible_from<HandlerTy, Args...>)
    auto emplace_handler (Args &&...args,
                          std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        -> event_handler_ref
    {
        m_handler.emplace<HandlerTy>(memres, std::forward<Args>(args)...);
        return m_handler;
    }

    /**
     * @brief Get the stop token used for coordinating shutdown.
     * @return `std::stop_token` associated with the internal `std::stop_source`.
     */
    auto get_token () const noexcept -> std::stop_token
    {
        return m_stop.get_token();
    }

    /**
     * @brief Request stop for the event handler.
     *
     * Interrupts libusb's event handling using `libusb_interrupt_event_handler`
     * and then requests stop on the internal `std::stop_source`.
     */
    auto request_stop ()
    {
        libusb_interrupt_event_handler(m_usb_ctx);
        m_stop.request_stop();
    }

    /**
     * @brief Get a reference to the stored event handler.
     * @return `event_handler_ref` for the handler owned by this service.
     */
    auto handler () -> event_handler_ref
    {
        return m_handler;
    }

    /**
     * @brief Start the stored event handler.
     * @return `true` on successful start.
     */
    auto start () -> bool
    {
        return m_handler.start(m_usb_ctx, m_stop.get_token());
    }

    /**
     * @brief Shutdown hook invoked by the base service.
     *
     * Requests stop and waits for handler shutdown via `m_handler.stop()`.
     */
    auto shutdown () -> void override
    {
        m_stop.request_stop();
        m_handler.stop();
    }

    /**
     * @brief Access the underlying libusb context.
     * @return Pointer to `libusb_context`.
     */
    auto usb_context () -> libusb_context *
    {
        return m_usb_ctx;
    }

  private:
    any_event_handler m_handler;
    std::stop_source m_stop;
    libusb_context *m_usb_ctx;
};

/**
 * @brief Retrieve the `handler_service` from a given executor.
 *
 * @param exec Executor reference to query.
 * @return Reference to the `handler_service` instance.
 * @throws std::runtime_error if the service is not present.
 */
inline auto get_handler_service (boost::capy::executor_ref exec) -> handler_service &
{
    handler_service *srv = exec.context().find_service<handler_service>();
    if (!srv)
        throw std::runtime_error{"co_usb handler service is not present"};
    return *srv;
}

/**
 * @brief Retrieve the `handler_service` from a given executor.
 *
 * @param exec Executor reference to query.
 * @return Reference to the `handler_service` instance.
 * @throws whatever can find_service throw.
 */
inline auto nothrow_get_handler_service (boost::capy::executor_ref exec) -> handler_service *
{
    handler_service *srv = exec.context().find_service<handler_service>();
    return srv;
}

} // namespace co_usb::ev::detail
