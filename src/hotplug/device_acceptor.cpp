#include "co_usb/device_ref.hpp"
#include "co_usb/error.hpp"
#include <boost/capy/io_result.hpp>
#include <boost/capy/io_task.hpp>
#include <co_usb/hotplug/device_acceptor.hpp>
#include <coroutine>
#include <libusb-1.0/libusb.h>
#include <mutex>
#include <system_error>

/**
 * @brief Internal awaitable type for @ref device_acceptor
 *
 * @details The awaitable checks if acceptor's map has a requested device:
 * @li if it does and the device is connected, it waits until it reconnects
 * @li if it does and the device is not connected, it immedeately resumes with the device retrieved
 * @li if it doesn't, it waits until the device connects
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
        if (!acceptor->m_dev_states.contains(triplet))
        {
            lock.unlock();
            return false;
        }
        auto &state = acceptor->m_dev_states.at(triplet);
        lock.unlock();
        return true;
    }

    std::coroutine_handle<> await_suspend (std::coroutine_handle<> h,
                                           boost::capy::io_env const *env)
    {
        std::unique_lock lock{acceptor->m_mutex};
        auto &state = acceptor->m_dev_states[triplet];
        state.env   = env;
        state.cont  = {h};
        lock.unlock();
        return std::noop_coroutine();
    }

    boost::capy::io_result<co_usb::device_ref> await_resume ()
    {
        std::unique_lock lock{acceptor->m_mutex};
        auto dev = acceptor->m_dev_states[triplet].dev;
        acceptor->m_dev_states.erase(triplet);
        lock.unlock();
        return {std::error_code{}, dev};
    }

    co_usb::device_acceptor *acceptor;
    device_triplet triplet;
};

static constexpr auto is_wild = [] (int v) { return v == LIBUSB_HOTPLUG_MATCH_ANY; };

bool co_usb::device_acceptor::triplet_comparator::operator()(const device_triplet &lhs,
                                                             const device_triplet &rhs) const
{
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
            auto &self = *(device_acceptor *)user_data;
            libusb_device_descriptor desc;
            libusb_get_device_descriptor(dev, &desc);
            device_triplet triplet{
                .vid = desc.idVendor, .pid = desc.idProduct, .dev_class = desc.bDeviceClass};
            {
                std::unique_lock lock{self.m_mutex};
                bool scheduled_before = self.m_dev_states.contains(triplet);
                auto &state           = self.m_dev_states[triplet];
                state.dev             = device_ref{dev};
                if (scheduled_before)
                {
                    auto &state = self.m_dev_states.at(triplet);
                    state.env->executor.post(state.cont);
                    state.env = nullptr;
                }
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
    libusb_hotplug_deregister_callback(m_ctx, m_handle);
}
