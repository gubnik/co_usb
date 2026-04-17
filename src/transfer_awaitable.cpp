#include <co_usb/transfer_awaitable.hpp>
#include <libusb-1.0/libusb.h>

struct transfer_env
{
    boost::capy::io_env const *io_env;
    boost::capy::continuation cont;
};

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
    io_env = env;
    cont   = {h};
    transfer_env *d =
        (transfer_env *)env->frame_allocator->allocate(sizeof(transfer_env), alignof(transfer_env));
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

boost::capy::io_result<size_t> co_usb::transfer_awaitable::await_resume ()
{
    io_env->frame_allocator->deallocate(transfer->user_data, sizeof(transfer_env),
                                        alignof(transfer_env));
    switch (transfer->status)
    {
    case LIBUSB_TRANSFER_COMPLETED: return {std::error_code{}, (size_t)transfer->actual_length};
    case LIBUSB_TRANSFER_TIMED_OUT:
        return {std::make_error_code(std::errc::timed_out), (size_t)transfer->actual_length};
    case LIBUSB_TRANSFER_CANCELLED:
        return {std::make_error_code(std::errc::operation_canceled),
                (size_t)transfer->actual_length};
    case LIBUSB_TRANSFER_NO_DEVICE:
        return {std::make_error_code(std::errc::no_such_device), (size_t)transfer->actual_length};
    case LIBUSB_TRANSFER_STALL:
        return {std::make_error_code(std::errc::device_or_resource_busy),
                (size_t)transfer->actual_length};
    case LIBUSB_TRANSFER_OVERFLOW:
        return {std::make_error_code(std::errc::no_buffer_space), (size_t)transfer->actual_length};
    case LIBUSB_TRANSFER_ERROR:
    default: return {std::make_error_code(std::errc::io_error), (size_t)transfer->actual_length};
    }
}
