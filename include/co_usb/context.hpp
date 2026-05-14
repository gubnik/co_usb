#pragma once

#include "co_usb/detail/service.hpp"
#include <boost/capy/concept/executor.hpp>
#include <boost/capy/ex/this_coro.hpp>
#include <boost/capy/task.hpp>
#include <concepts>
#include <ranges>
#include <stdexcept>
#include <stop_token>

namespace co_usb
{

enum class use_service
{
    no = 0,
    yes
};

/**
 * @brief default handler function for the service
 */
void default_handler(libusb_context *ctx, std::stop_token st);

/**
 * @brief libusb_context wrapper
 *
 * @tparam Service whether to create a handler service or not
 */
template <use_service Service = use_service::yes> struct context;

template <> struct context<use_service::yes>
{
  private:
    template <boost::capy::Executor Exec,
              std::invocable<libusb_context *, std::stop_token> HandlerFn>
    void init (Exec &&exec, HandlerFn &&handler_fn)
    {
        libusb_context *ctx;
        auto r = libusb_init(&ctx);
        if (r != LIBUSB_SUCCESS)
        {
            throw std::runtime_error{"Cannot initialize libusb"};
        }
        m_ctx = {ctx, [] (libusb_context *ctx) { libusb_exit(ctx); }};
        detail::handler_service &service =
            std::forward<Exec>(exec).context().template use_service<detail::handler_service>();
        service.start_thread(m_ctx, std::forward<HandlerFn>(handler_fn));
        m_ss = service.stop_source();
    }

  public:
    template <boost::capy::Executor Exec> explicit context (Exec &&exec)
    {
        init(std::forward<Exec>(exec), default_handler);
    }

    template <boost::capy::Executor Exec,
              std::invocable<libusb_context *, std::stop_token> HandlerFn>
    explicit context (Exec &&exec, HandlerFn &&handler_fn)
    {
        init(std::forward<Exec>(exec), std::forward<HandlerFn>(handler_fn));
    }

    template <typename R, boost::capy::Executor Exec,
              std::invocable<libusb_context *, std::stop_token> HandlerFn>
        requires std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, libusb_option>
    explicit context(R &&options, Exec &&exec, HandlerFn &&handler_fn)
    {
        init(std::forward<Exec>(exec), std::forward<HandlerFn>(handler_fn));
        for (auto opt : options)
        {
            libusb_set_option(m_ctx, opt);
        }
    }

    context(context const &) = delete;
    context(context &&)      = delete;

    context &operator=(context const &) = delete;
    context &operator=(context &&)      = delete;

    auto *get () const noexcept
    {
        return m_ctx.get();
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
    std::shared_ptr<libusb_context> m_ctx;
    std::stop_source m_ss;
};

template <> struct context<use_service::no>
{
  private:
    void init();

  public:
    explicit context();

    template <typename R>
        requires std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, libusb_option>
    explicit context(R &&options)
    {
        init();
        for (auto opt : options)
        {
            libusb_set_option(m_ctx, opt);
        }
    }

    ~context();

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
