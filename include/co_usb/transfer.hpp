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

struct transfer_awaitable
{
    transfer_awaitable(libusb_transfer* tfer, libusb_device_handle *devh, boost::capy::mutable_buffer mb)
    : transfer(tfer), devh(devh), buffer(mb)
    {

    }

    bool await_ready() noexcept
    {
        return false;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h, boost::capy::io_env const* env)
    {
        std::println("Entering suspend");
        cont = {h};
        transfer_env d{.io_env = env, .cont = cont};
        libusb_fill_control_transfer(transfer, devh, (unsigned char*)buffer.data(),
            [](libusb_transfer *transfer){
                auto *d = (transfer_env*)transfer->user_data;
                d->io_env->executor.post(d->cont);
                },
            &d, 0);
        libusb_submit_transfer(transfer);
        return std::noop_coroutine();
    }

    boost::capy::io_result<size_t> await_resume()
    {
        std::println("Resumed");
        return {std::make_error_code(std::errc::bad_address), 1};
    }
    
    libusb_transfer *transfer = nullptr;
    libusb_device_handle *devh;
    boost::capy::mutable_buffer buffer;
    boost::capy::continuation cont;
};

static_assert(boost::capy::IoAwaitable<transfer_awaitable>);


}
