#pragma once

#include "co_usb/service.hpp"
#include <boost/capy/concept/executor.hpp>
#include <boost/capy/ex/this_coro.hpp>
#include <boost/capy/task.hpp>
#include <concepts>
#include <libusb-1.0/libusb.h>
#include <ranges>
#include <stdexcept>
#include <stop_token>
#include <type_traits>
#include <utility>

namespace co_usb
{

enum class use_service
{
    no = 0,
    yes
};

/**
 * @brief libusb_context wrapper
 *
 * @tparam Service whether to create a handler service or not
 */
template <use_service Service = use_service::yes> struct context;

template <> struct context<use_service::yes>
{
    explicit context (boost::capy::Executor auto &&exec)
    {
        auto r = libusb_init(&m_ctx);
        if (r != LIBUSB_SUCCESS)
        {
            throw std::runtime_error{"Cannot initialize libusb"};
        }
        detail::handler_service &service =
            exec.context().template use_service<detail::handler_service>();
        service.start_thread(m_ctx, detail::handler_service::default_handler);
        m_ss = service.stop_source();
    }

    template <std::invocable<libusb_context *, std::stop_token> HandlerFn>
    explicit context (boost::capy::Executor auto &&exec, HandlerFn &&handler_fn)
    {
        auto r = libusb_init(&m_ctx);
        if (r != LIBUSB_SUCCESS)
        {
            throw std::runtime_error{"Cannot initialize libusb"};
        }
        detail::handler_service &service =
            exec.context().template use_service<detail::handler_service>();
        service.start_thread(m_ctx, std::forward<HandlerFn>(handler_fn));
        m_ss = service.stop_source();
    }

    template <typename R, std::invocable<libusb_context *, std::stop_token> HandlerFn>
        requires std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, libusb_option>
    explicit context(R &&options, boost::capy::Executor auto &&exec, HandlerFn &&handler_fn)
    {
        auto r = libusb_init(&m_ctx);
        if (r != LIBUSB_SUCCESS)
        {
            throw std::runtime_error{"Cannot initialize libusb"};
        }
        detail::handler_service &service =
            exec.context().template use_service<detail::handler_service>();
        service.start_thread(m_ctx, std::forward<HandlerFn>(handler_fn));
        m_ss = service.stop_source();
        for (auto opt : options)
        {
            libusb_set_option(m_ctx, opt);
        }
    }

    ~context ()
    {
        if (m_ctx)
            libusb_exit(m_ctx);
    }

    context(context const &) = delete;
    context(context &&)      = delete;

    context &operator=(context const &) = delete;
    context &operator=(context &&)      = delete;

    auto *get () const noexcept
    {
        return m_ctx;
    }

    auto request_stop ()
    {
        m_ss.request_stop();
    }

    auto get_token ()
    {
        return m_ss.get_token();
    }

  private:
    libusb_context *m_ctx;
    std::stop_source m_ss;
};

template <> struct context<use_service::no>
{

    explicit context ()
    {
        auto r = libusb_init(&m_ctx);
        if (r != LIBUSB_SUCCESS)
        {
            throw std::runtime_error{"Cannot initialize libusb"};
        }
    }

    template <typename R>
        requires std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, libusb_option>
    explicit context(R &&options)
    {
        auto r = libusb_init(&m_ctx);
        if (r != LIBUSB_SUCCESS)
        {
            throw std::runtime_error{"Cannot initialize libusb"};
        }
        for (auto opt : options)
        {
            libusb_set_option(m_ctx, opt);
        }
    }

    ~context ()
    {
        if (m_ctx)
            libusb_exit(m_ctx);
    }

    context(context const &) = delete;
    context(context &&)      = delete;

    context &operator=(context const &) = delete;
    context &operator=(context &&)      = delete;

    auto *get () const noexcept
    {
        return m_ctx;
    }

  private:
    libusb_context *m_ctx;
};

} // namespace co_usb
