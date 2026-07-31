#pragma once

#include "co_usb/ev/detail/handler_service.hpp"
#include "co_usb/ev/event_handler_ref.hpp"
#include "co_usb/usb_error.hpp"
#include "co_usb/wrapper/device_ref.hpp"
#include "co_usb/wrapper/device_triplet.hpp"
#include <algorithm>
#include <boost/capy/concept/io_awaitable.hpp>
#include <boost/capy/continuation.hpp>
#include <boost/capy/ex/async_mutex.hpp>
#include <boost/capy/ex/executor_ref.hpp>
#include <boost/capy/ex/io_env.hpp>
#include <boost/capy/ex/run.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/ex/this_coro.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/io_task.hpp>
#include <boost/capy/task.hpp>
#include <coroutine>
#include <libusb.h>
#include <list>
#include <memory_resource>
#include <mutex>
#include <stop_token>
#include <system_error>
#include <utility>

namespace co_usb::hotplug
{

/**
 * @ingroup hotplug
 *
 * @brief Asio/Corosio-like acceptor for devices via hotplug API.
 *
 * @details Use this class to create a structured accept loop for your application.
 * The acceptor allows for foregoing suspension entirely if the device arrived before `acccept` was
 * called.
 */
struct device_acceptor
{
    explicit device_acceptor (boost::capy::executor_ref exec,
                              std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        : m_ev_handler_ref(::co_usb::ev::detail::get_handler_service(exec).handler()),
          m_usb_ctx(::co_usb::ev::detail::get_handler_service(exec).usb_context()),
          m_memres(memres), m_resumptions(memres), m_arrived_devices(memres), m_handle(0)
    {
    }

    ~device_acceptor ()
    {
        shutdown();
    }

    auto shutdown () -> void
    {
        libusb_hotplug_callback_handle handle{0};
        {
            std::unique_lock<std::mutex> lock{m_mutex};
            handle = std::exchange(m_handle, 0);
            if (handle == 0)
                return;
        }
        libusb_hotplug_deregister_callback(m_usb_ctx, handle);

        std::unique_lock lock{m_mutex};
        for (resumption_t *res : m_resumptions)
        {
            lock.unlock();
            res->op_res->ec = std::make_error_code(std::errc::operation_canceled);
            res->env->executor.post(res->cont);
            lock.lock();
        }
        m_resumptions.clear();
        m_arrived_devices.clear();
    }

    auto bind (device_triplet triplet, std::error_code &ec) -> void
    {
        m_filter = triplet;
        ec.clear();
    }

    [[nodiscard]] auto bind (device_triplet triplet) -> std::error_code
    {
        m_filter = triplet;
        return {};
    }

    auto listen () -> std::error_code
    {
        if (m_handle != 0)
        {
            return std::make_error_code(std::errc::operation_in_progress);
        }
        libusb_hotplug_callback_handle handle;
        auto r = libusb_hotplug_register_callback(
            m_usb_ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE, m_filter.vid,
            m_filter.pid, m_filter.dev_class,
            [] (libusb_context *ctx, libusb_device *dev, libusb_hotplug_event ev,
                void *user_data) -> int
            {
                device_ref dev_ref{dev};
                device_acceptor &self = *static_cast<device_acceptor *>(user_data);
                std::unique_lock lock{self.m_mutex};
                if (self.m_resumptions.empty())
                {
                    self.m_arrived_devices.emplace_back(device_ref{dev});
                    return 0;
                }
                resumption_t *r = self.m_resumptions.front();
                r->op_res->dev_ref = device_ref{dev};
                r->env->executor.post(r->cont);
                self.m_resumptions.erase(self.m_resumptions.begin());
                return 0;
            },
            this, &handle);
        if (r != LIBUSB_SUCCESS)
        {
            return make_usb_error_code(static_cast<usb_error>(r));
        }
        m_handle = handle;
        return {};
    }

    auto close () -> std::error_code
    {
        if (m_handle == 0)
        {
            return std::make_error_code(std::errc::invalid_argument);
        }
        libusb_hotplug_deregister_callback(m_usb_ctx, m_handle);
        return {};
    }

    auto accept () -> boost::capy::io_task<device_ref>
    {
        auto exec = co_await boost::capy::this_coro::executor;
        auto stop = co_await boost::capy::this_coro::stop_token;
        auto alloc = co_await boost::capy::this_coro::frame_allocator;
        op_result op_res{};
        resumption_t res{};

        auto stop_fn = [this, op_res_ptr = &op_res, res = &res] () mutable
        {
            std::unique_lock lock{m_mutex};
            if (res->env && res->cont.h)
            {
                res->op_res->ec = std::make_error_code(std::errc::operation_canceled);
                res->env->executor.post(res->cont);
                auto it =
                    std::find_if(m_resumptions.begin(), m_resumptions.end(),
                                 [res_ptr = res] (resumption_t *res) { return res == res_ptr; });
                if (it != m_resumptions.end())
                {
                    m_resumptions.erase(it);
                }
            }
            return;
        };

        std::stop_callback stop_cb{stop, std::move(stop_fn)};
        co_return co_await awaitable(this, &op_res, &res);
    }

    device_acceptor(const device_acceptor &) = delete;
    device_acceptor &operator=(const device_acceptor &) = delete;
    device_acceptor(device_acceptor &&) = delete;
    device_acceptor &operator=(device_acceptor &&) = delete;

  private:
    struct op_result
    {
        device_ref dev_ref{};
        std::error_code ec{};
    };

    struct resumption_t
    {
        boost::capy::io_env const *env{nullptr};
        boost::capy::continuation cont{.h = nullptr};
        op_result *op_res{nullptr};
    };

    struct awaitable
    {
        explicit awaitable (device_acceptor *acceptor, op_result *state, resumption_t *res)
            : acceptor(acceptor), op_res(state), res(res)
        {
        }

        inline bool await_ready ()
        {
            std::unique_lock lock{acceptor->m_mutex};
            if (!acceptor->m_arrived_devices.empty())
            {
                op_res->dev_ref = acceptor->m_arrived_devices.back();
                acceptor->m_arrived_devices.pop_back();
                return true;
            }
            return false;
        }

        inline std::coroutine_handle<> await_suspend (std::coroutine_handle<> h,
                                                      boost::capy::io_env const *env)
        {
            if (env->stop_token.stop_requested())
            {
                std::unique_lock lock{acceptor->m_mutex};
                op_res->ec = std::make_error_code(std::errc::operation_canceled);
                return h;
            }

            std::unique_lock lock{acceptor->m_mutex};
            if (!acceptor->m_arrived_devices.empty())
            {
                op_res->dev_ref = acceptor->m_arrived_devices.back();
                acceptor->m_arrived_devices.pop_back();
                return h;
            }

            *res = resumption_t{env, boost::capy::continuation{h}, op_res};
            acceptor->m_resumptions.emplace_back(res);
            return std::noop_coroutine();
        }

        inline boost::capy::io_result<device_ref> await_resume ()
        {
            std::unique_lock lock{acceptor->m_mutex};
            if (op_res->ec)
            {
                return {op_res->ec, device_ref{}};
            }
            return {std::error_code{}, op_res->dev_ref};
        }

        device_acceptor *acceptor;
        op_result *op_res;
        resumption_t *res;
    };

    ev::event_handler_ref m_ev_handler_ref;
    std::mutex m_mutex;
    std::pmr::memory_resource *m_memres;

    std::pmr::list<resumption_t *> m_resumptions{};
    std::pmr::list<device_ref> m_arrived_devices{};

    device_triplet m_filter{};
    libusb_context *m_usb_ctx;
    libusb_hotplug_callback_handle m_handle{0};
};

} // namespace co_usb::hotplug
