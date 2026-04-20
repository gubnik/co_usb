#include "co_usb/tfer/error.hpp"
#include <co_usb/tfer/transfer_awaitable.hpp>
#include <libusb-1.0/libusb.h>

co_usb::transfer_awaitable::transfer_awaitable (libusb_transfer *tfer) noexcept : transfer(tfer)
{
}

bool co_usb::transfer_awaitable::await_ready () noexcept
{
    return false;
}

std::coroutine_handle<> co_usb::transfer_awaitable::await_suspend (std::coroutine_handle<> h,
                                                                   boost::capy::io_env const *env)
{
    if (env->stop_token.stop_requested())
    {
        libusb_cancel_transfer(transfer);
        return h;
    }
    tfer_env.io_env     = env;
    tfer_env.cont       = {h};
    transfer->user_data = &tfer_env;
    transfer->callback  = [] (libusb_transfer *tfer)
    {
        transfer_env *tv = (transfer_env *)tfer->user_data;
        tv->io_env->executor.post(tv->cont);
    };
    auto r = libusb_submit_transfer(transfer);
    if (r != LIBUSB_SUCCESS)
    {
        transfer->status        = LIBUSB_TRANSFER_ERROR;
        transfer->actual_length = 0;
        return h;
    }
    return std::noop_coroutine();
}

boost::capy::io_result<size_t> co_usb::transfer_awaitable::await_resume ()
{
    return {make_transfer_status(transfer->status), static_cast<size_t>(transfer->actual_length)};
}
