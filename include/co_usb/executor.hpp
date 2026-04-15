#pragma once

#include "boost/capy/ex/execution_context.hpp"
#include "boost/capy/ex/frame_allocator.hpp"
#include <bits/types/struct_timeval.h>
#include <coroutine>
#include <libusb-1.0/libusb.h>
#include <queue>
#include <stdexcept>
#include <stop_token>
#include <thread>

namespace co_usb
{

struct handler_loop : public boost::capy::execution_context
{
    struct executor_type;

    handler_loop () : boost::capy::execution_context(this)
    {
        auto ret = libusb_init(&m_ctx);
        if (ret < 0)
            throw std::runtime_error{"Cannot initialize usb context"};
        m_thread_opt = std::jthread(
            [&] (std::stop_token st)
            {
                timeval tv;
                tv.tv_sec  = 0;
                tv.tv_usec = 10'000;
                while (!st.stop_requested())
                {
                    libusb_handle_events_timeout(m_ctx, &tv);
                    while (!m_queue.empty() && !st.stop_requested())
                    {
                        auto h = m_queue.front();
                        m_queue.pop();
                        boost::capy::safe_resume(h);
                    }
                }
            });
    }

    ~handler_loop ()
    {
        shutdown();
        destroy();
        libusb_exit(m_ctx);
    }

    libusb_context *usb_context () noexcept
    {
        return m_ctx;
    }

    void run ()
    {
        m_thread_opt->join();
    }

    void enqueue (std::coroutine_handle<> h)
    {
        m_queue.push(h);
    }

    bool is_running_on_this_thread () const noexcept
    {
        return std::this_thread::get_id() == m_owner;
    }

    executor_type get_executor() noexcept;

  private:
    libusb_context *m_ctx;
    std::optional<std::jthread> m_thread_opt;
    std::thread::id m_owner;
    std::stop_token m_st;
    std::queue<std::coroutine_handle<>> m_queue;
};

class handler_loop::executor_type
{
    friend class handler_loop;
    handler_loop *loop_ = nullptr;

    explicit executor_type (handler_loop &loop) noexcept : loop_(&loop)
    {
    }

  public:
    executor_type() = default;

    boost::capy::execution_context &context () const noexcept
    {
        return *loop_;
    }

    void on_work_started () const noexcept
    {
    }
    void on_work_finished () const noexcept
    {
    }

    std::coroutine_handle<> dispatch (boost::capy::continuation &c) const
    {
        if (loop_->is_running_on_this_thread())
            return c.h;
        loop_->enqueue(c.h);
        return std::noop_coroutine();
    }

    void post (boost::capy::continuation &c) const
    {
        loop_->enqueue(c.h);
    }

    bool operator==(executor_type const &other) const noexcept
    {
        return loop_ == other.loop_;
    }
};

inline handler_loop::executor_type handler_loop::get_executor () noexcept
{
    return executor_type{*this};
}

} // namespace co_usb
