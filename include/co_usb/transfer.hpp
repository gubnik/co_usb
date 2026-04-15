#pragma once

#include "boost/capy/concept/io_awaitable.hpp"
#include "boost/capy/continuation.hpp"
#include "boost/capy/ex/io_env.hpp"
#include "boost/capy/io_result.hpp"
#include <libusb-1.0/libusb.h>

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
 * This is the lowest possible representation level of an asynchronous unit of
 * libusb. It does not care for any of a transfer's properties and does not have
 * direction- or transfer type-related bevahioural differences.
 *
 * You should use this awaitable for porting an existing codebase to co_usb or
 * when you need ultimate control.
 *
 * @note IMPORTANT user_data and callback fields of a transfer object ARE
 * OVERRIDDEN on await_suspend with internal coroutine machinery, to access
 * data from a completed transfer consider inspecting the transfer object after
 * the completion of the awaitable.
 */
struct transfer_awaitable
{
    transfer_awaitable (libusb_transfer *tfer, libusb_device_handle *devh)
        : transfer(tfer), devh(devh)
    {
    }

    bool await_ready () noexcept
    {
        return false;
    }

    std::coroutine_handle<> await_suspend (std::coroutine_handle<> h,
                                           boost::capy::io_env const *env)
    {
        if (env->stop_token.stop_requested())
            return h;
        io_env          = env;
        cont            = {h};
        transfer_env *d = (transfer_env *)env->frame_allocator->allocate(
            sizeof(transfer_env), alignof(transfer_env));
        new (d) transfer_env(env, cont);
        transfer->user_data = d;
        transfer->callback  = [] (libusb_transfer *tfer)
        {
            transfer_env *tv = (transfer_env *)tfer->user_data;
            tv->io_env->executor.post(tv->cont);
        };
        libusb_submit_transfer(transfer);
        return std::noop_coroutine();
    }

    boost::capy::io_result<size_t> await_resume ()
    {
        io_env->frame_allocator->deallocate(
            transfer->user_data, sizeof(transfer_env), alignof(transfer_env));
        if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
            return {std::error_code{}, (size_t)transfer->actual_length};
        switch (transfer->status)
        {
        case LIBUSB_TRANSFER_TIMED_OUT:
            return {std::make_error_code(std::errc::timed_out),
                    (size_t)transfer->actual_length};
        case LIBUSB_TRANSFER_CANCELLED:
            return {std::make_error_code(std::errc::operation_canceled),
                    (size_t)transfer->actual_length};
        case LIBUSB_TRANSFER_NO_DEVICE:
            return {std::make_error_code(std::errc::no_such_device),
                    (size_t)transfer->actual_length};
        case LIBUSB_TRANSFER_STALL:
            return {std::make_error_code(std::errc::device_or_resource_busy),
                    (size_t)transfer->actual_length};
        case LIBUSB_TRANSFER_OVERFLOW:
            return {std::make_error_code(std::errc::no_buffer_space),
                    (size_t)transfer->actual_length};
        case LIBUSB_TRANSFER_ERROR:
        default:
            return {std::make_error_code(std::errc::io_error),
                    (size_t)transfer->actual_length};
        }
    }

    libusb_transfer *transfer         = nullptr;
    libusb_device_handle *devh        = nullptr;
    boost::capy::io_env const *io_env = nullptr;
    boost::capy::continuation cont;
};

static_assert(boost::capy::IoAwaitable<transfer_awaitable>);

} // namespace co_usb
