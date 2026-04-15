#pragma once

#include "boost/capy/buffers.hpp"
#include "boost/capy/concept/io_awaitable.hpp"
#include "boost/capy/continuation.hpp"
#include "boost/capy/ex/io_env.hpp"
#include "boost/capy/io_result.hpp"
#include <coroutine>
#include <libusb-1.0/libusb.h>
#include <print>

namespace co_usb
{

struct transfer_env
{
    boost::capy::io_env const *io_env;
    boost::capy::continuation cont;
};

/**
 * @brief Awaitable for submitting transfers
 *
 * @note Overrides user-provided user_data and callback
 */
struct transfer_awaitable
{
    transfer_awaitable(libusb_transfer* tfer, libusb_device_handle *devh)
    : transfer(tfer), devh(devh)
    {

    }

    bool await_ready() noexcept
    {
        return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h, boost::capy::io_env const* env)
    {
        io_env = env;
        std::println("Entering suspend");
        cont = {h};
        transfer_env *d = (transfer_env*) env->frame_allocator->allocate(sizeof(transfer_env), alignof(transfer_env));
        new (d) transfer_env(env, cont);
        // TODO: consider not overriding
        transfer->user_data = d;
        transfer->callback = [](libusb_transfer* tfer){
            transfer_env *tv = (transfer_env*)tfer->user_data;
            tv->io_env->executor.post(tv->cont);
            std::println("Callback was called and reached its end!");
        };
        libusb_submit_transfer(transfer);
        return std::noop_coroutine();
    }

    boost::capy::io_result<size_t> await_resume()
    {
        std::println("Resumed");
        io_env->frame_allocator->deallocate(transfer->user_data, sizeof(transfer_env), alignof(transfer_env));
        return {std::error_code{}, (size_t)transfer->actual_length};
    }
    
    libusb_transfer *transfer = nullptr;
    libusb_device_handle *devh = nullptr;
    boost::capy::io_env const* io_env = nullptr;
    boost::capy::continuation cont;
};

static_assert(boost::capy::IoAwaitable<transfer_awaitable>);

}
