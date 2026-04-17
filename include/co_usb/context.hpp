#pragma once

#include <boost/capy/ex/this_coro.hpp>
#include <boost/capy/task.hpp>
#include <concepts>
#include <libusb-1.0/libusb.h>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <stop_token>
#include <thread>

namespace co_usb
{

struct context
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
        if (m_opt_handler)
            m_opt_handler->join();
        if (m_ctx)
            libusb_exit(m_ctx);
    }

    context(context const &) = delete;
    context(context &&)      = delete;

    context &operator=(context const &) = delete;
    context &operator=(context &&)      = delete;

    auto *get () noexcept
    {
        return m_ctx;
    }

    auto *const get () const noexcept
    {
        return m_ctx;
    }

    auto spawn_handler_thread (timeval tv = {.tv_sec = 0, .tv_usec = 10'000})
    {
        m_opt_handler = std::thread{[st = m_ss.get_token(), tv, this] ()
                                    {
                                        timeval _tv = tv;
                                        while (!st.stop_requested())
                                        {
                                            libusb_handle_events_timeout(m_ctx, &_tv);
                                        }
                                    }};
    }

    boost::capy::task<void> spawn_handler (timeval tv = {.tv_sec = 0, .tv_usec = 10'000})
    {
        auto st = co_await boost::capy::this_coro::stop_token;
        while (!st.stop_requested())
        {
            libusb_handle_events_timeout(m_ctx, &tv);
        }
    }

    auto stop_handler ()
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
    std::optional<std::thread> m_opt_handler;
};

} // namespace co_usb
