#pragma once

#include "boost/capy/concept/io_awaitable.hpp"
#include "boost/capy/continuation.hpp"
#include "boost/capy/ex/io_env.hpp"
#include "boost/capy/io_result.hpp"
#include <libusb-1.0/libusb.h>

namespace co_usb
{

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
    transfer_awaitable(libusb_transfer *tfer) noexcept;

    bool await_ready() noexcept;

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h,
                                          boost::capy::io_env const *env);

    boost::capy::io_result<size_t> await_resume();

    libusb_transfer *transfer = nullptr;
    struct transfer_env
    {
        boost::capy::io_env const *io_env = nullptr;
        boost::capy::continuation cont;
    } tfer_env;
};

static_assert(boost::capy::IoAwaitable<transfer_awaitable>);

} // namespace co_usb
