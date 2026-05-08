#include "co_usb/device_ref.hpp"
#include "co_usb/error.hpp"
#include <boost/capy/continuation.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/io_task.hpp>
#include <co_usb/hotplug/device_acceptor.hpp>
#include <coroutine>
#include <libusb-1.0/libusb.h>
#include <mutex>
#include <system_error>

/**
 * @brief Internal awaitable type for @ref device_acceptor
 */
struct co_usb::device_acceptor::acceptor_awaitable
{
    acceptor_awaitable (device_acceptor *acceptor, device_triplet triplet)
        : acceptor(acceptor), triplet(triplet)
    {
    }

    bool await_ready ()
    {
        std::unique_lock lock{acceptor->m_mutex};
        auto it = acceptor->m_dev_states.find(triplet);
        if (it == acceptor->m_dev_states.end())
        {
            return false;
        }

        auto const &state = it->second;
        // there is already a pending awaitable - acceptor does not support multiple accept points,
        // don't suspend and set an error
        if (state.env)
        {
            err = usb_error::resource_busy;
        }
        return true;
    }

    std::coroutine_handle<> await_suspend (std::coroutine_handle<> h,
                                           boost::capy::io_env const *env)
    {
        // resume if the operation was cancelled
        if (env->stop_token.stop_requested())
        {
            err = usb_error::interrupted;
            return h;
        }

        std::unique_lock lock{acceptor->m_mutex};

        // insert a device entry and set resumption point
        auto &state = acceptor->m_dev_states[triplet];

        // resume if the device arrived while we were locking
        if (state.env || state.dev.get())
        {
            lock.unlock();
            return h;
        }

        state.env  = env;
        state.cont = {h};

        // set up an observer to fire when the accepting coroutine
        // was cancelled to resume and not wait for a hotplug
        state.opt_cb.emplace(env->stop_token,
                             [acceptor = acceptor, triplet = triplet] ()
                             {
                                 std::unique_lock lock{acceptor->m_mutex};
                                 auto it = acceptor->m_dev_states.find(triplet);
                                 if (it == acceptor->m_dev_states.end() || !it->second.env)
                                 {
                                     return;
                                 }
                                 auto &state = it->second;
                                 // clear callback
                                 state.opt_cb.reset();
                                 state.err = usb_error::interrupted;
                                 auto env  = state.env;
                                 auto cont = state.cont;
                                 state.env = nullptr;
                                 lock.unlock();
                                 env->executor.post(cont);
                             });

        return std::noop_coroutine();
    }

    boost::capy::io_result<co_usb::device_ref> await_resume ()
    {
        std::unique_lock lock{acceptor->m_mutex};
        auto it = acceptor->m_dev_states.find(triplet);

        // if resumed with no device state - something went wrong
        if (it == acceptor->m_dev_states.end())
        {
            return {make_usb_error_code(usb_error::no_device), device_ref{}};
        }

        // await_suspend and await_ready error take precedence
        if (err != usb_error::success)
        {
            return {make_usb_error_code(err), device_ref{}};
        }

        auto &state = it->second;
        auto dev    = state.dev;
        auto err    = state.err;
        acceptor->m_dev_states.erase(it);
        lock.unlock();
        return {make_usb_error_code(err), dev};
    }

    co_usb::device_acceptor *acceptor;
    device_triplet triplet;
    usb_error err = usb_error::success;
};

bool co_usb::device_acceptor::triplet_comparator::operator()(const device_triplet &lhs,
                                                             const device_triplet &rhs) const
{
    constexpr auto is_wild = [] (int v) { return v == LIBUSB_HOTPLUG_MATCH_ANY; };
    if (!is_wild(lhs.vid) && !is_wild(rhs.vid) && lhs.vid != rhs.vid)
    {
        return lhs.vid < rhs.vid;
    }
    if (!is_wild(lhs.pid) && !is_wild(rhs.pid) && lhs.pid != rhs.pid)
    {
        return lhs.pid < rhs.pid;
    }
    if (!is_wild(lhs.dev_class) && !is_wild(rhs.dev_class) && lhs.dev_class != rhs.dev_class)
    {
        return lhs.dev_class < rhs.dev_class;
    }
    return false;
}

co_usb::device_acceptor::device_acceptor (libusb_context *ctx,
                                          std::pmr::memory_resource *memory_resource)
    : m_ctx{ctx}, m_allocator(memory_resource), m_dev_states{m_allocator}
{
    auto r = libusb_hotplug_register_callback(
        m_ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE,
        LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
        [] (libusb_context *ctx, libusb_device *dev, libusb_hotplug_event ev,
            void *user_data) -> int
        {
            auto &self = *static_cast<device_acceptor *>(user_data);

            libusb_device_descriptor desc;
            libusb_get_device_descriptor(dev, &desc);
            device_triplet triplet{desc.idVendor, desc.idProduct, desc.bDeviceClass};

            std::unique_lock lock{self.m_mutex};
            auto &state = self.m_dev_states[triplet];
            state.dev   = device_ref{dev};

            if (state.env)
            {
                auto &state = self.m_dev_states.at(triplet);
                auto env    = state.env;
                auto cont   = state.cont;
                state.env   = nullptr;
                lock.unlock();
                env->executor.post(cont);
            }

            return 0;
        },
        this, &m_handle);
    if (r != LIBUSB_SUCCESS)
    {
        throw std::system_error{make_usb_error_code(static_cast<usb_error>(r))};
    }
}

boost::capy::io_task<co_usb::device_ref>
co_usb::device_acceptor::accept (co_usb::device_triplet triplet)
{
    co_return co_await acceptor_awaitable{this, triplet};
}

co_usb::device_acceptor::~device_acceptor ()
{
    std::vector<std::pair<boost::capy::io_env const *, boost::capy::continuation>> to_resume;
    { // hold the lock till we collect all the resumption points
        std::unique_lock lock{m_mutex};
        for (auto &[_, state] : m_dev_states)
        {
            if (!state.env)
            {
                continue;
            }
            state.err = usb_error::interrupted;
            to_resume.emplace_back(state.env, state.cont);
            state.opt_cb.reset();
        }
        m_dev_states.clear();
    }
    for (auto [env, cont] : to_resume)
    {
        env->executor.post(cont);
    }
    libusb_hotplug_deregister_callback(m_ctx, m_handle);
}
