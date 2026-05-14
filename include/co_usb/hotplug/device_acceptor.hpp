#pragma once

#include "co_usb/device_ref.hpp"
#include "co_usb/device_triplet.hpp"
#include "co_usb/error.hpp"
#include <boost/capy/continuation.hpp>
#include <boost/capy/ex/io_env.hpp>
#include <boost/capy/io_task.hpp>
#include <functional>
#include <libusb.h>
#include <map>
#include <memory_resource>
#include <mutex>
#include <stop_token>

namespace co_usb
{

/**
 * @brief Accepts devices via hotplug
 *
 * @note Totally unrelated to @ref hotplug_awaitable in any way
 *
 * @details This class maintains a pool of currently connected devices and performs the book keeping
 * to ensure that the same device is not accepted while already connected.
 *
 * @note There can only be a single accept point for each acceptor.
 *
 * Internal map is allocated using @ref std::pmr::polymorphic_allocator to allow allocator
 * propagation from coroutine frame allocator.
 */
struct device_acceptor
{
    explicit device_acceptor(libusb_context *ctx, std::pmr::memory_resource *memory_resource =
                                                      std::pmr::get_default_resource());

    ~device_acceptor();

    device_acceptor(const device_acceptor &)            = delete;
    device_acceptor &operator=(const device_acceptor &) = delete;
    device_acceptor(device_acceptor &&)                 = delete;
    device_acceptor &operator=(device_acceptor &&)      = delete;

    /**
     * @brief accepts a device by its triplet
     *
     * @details waits until a device with a provided triplet is attached.
     * Can avoid suspension if the device was attached before the accept call.
     *
     * @returns reference to an accepted device
     * @returns error code co_usb::usb_error::interrupted on cancellation or when acceptor dtor was
     * called
     * @returns error code co_usb::usb_error::no_device if device was detached while suspending &
     * acquiring the internal lock
     */
    boost::capy::io_task<device_ref> accept(device_triplet triplet);

  private:
    struct acceptor_awaitable;
    friend struct acceptor_awaitable;

    // internal device entry
    struct device_state_t
    {
        boost::capy::io_env const *env{nullptr};
        boost::capy::continuation cont{};
        device_ref dev{};
        usb_error err;

        // fires on cancellation to interrupt awaitables
        std::optional<std::stop_callback<std::function<void()>>> opt_cb;
    };

    struct triplet_comparator
    {
        bool operator()(const device_triplet &lhs, const device_triplet &rhs) const;
    };

    using allocator_t =
        std::pmr::polymorphic_allocator<std::pair<const device_triplet, device_state_t>>;
    using map_t = std::map<device_triplet, device_state_t, triplet_comparator, allocator_t>;

    libusb_context *m_ctx;
    libusb_hotplug_callback_handle m_handle;
    allocator_t m_allocator;
    std::mutex m_mutex;
    map_t m_dev_states;
};

} // namespace co_usb
