#include "co_usb/transfer/error.hpp"
#include <co_usb/transfer/transfer_awaitable.hpp>
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
    m_data.io_env       = env;
    m_data.cont         = {h};
    transfer->user_data = &m_data;
    transfer->callback  = [] (libusb_transfer *tfer)
    {
        cb_data *data = (cb_data *)tfer->user_data;
        data->io_env->executor.post(data->cont);
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
