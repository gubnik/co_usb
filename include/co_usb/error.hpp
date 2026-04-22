#pragma once

#include <libusb-1.0/libusb.h>
#include <system_error>

namespace co_usb
{

enum class usb_error : int
{
    success = LIBUSB_SUCCESS,

    /** Input/output error */
    io_error = LIBUSB_ERROR_IO,

    /** Invalid parameter */
    invalid_param = LIBUSB_ERROR_INVALID_PARAM,

    /** Access denied (insufficient permissions) */
    access_error = LIBUSB_ERROR_ACCESS,

    /** No such device (it may have been disconnected) */
    no_device = LIBUSB_ERROR_NO_DEVICE,

    /** Entity not found */
    not_found = LIBUSB_ERROR_NOT_FOUND,

    /** Resource busy */
    resource_busy = LIBUSB_ERROR_BUSY,

    /** Operation timed out */
    timed_out = LIBUSB_ERROR_TIMEOUT,

    /** Overflow */
    overflow = LIBUSB_ERROR_OVERFLOW,

    /** Pipe error */
    broken_pipe = LIBUSB_ERROR_PIPE,

    /** System call interrupted (perhaps due to signal) */
    interrupted = LIBUSB_ERROR_INTERRUPTED,

    /** Insufficient memory */
    out_of_memory = LIBUSB_ERROR_NO_MEM,

    /** Operation not supported or unimplemented on this platform */
    not_supported = LIBUSB_ERROR_NOT_SUPPORTED,

    /* NB: Remember to update LIBUSB_ERROR_COUNT below as well as the
       message strings in strerror.c when adding new error codes here. */

    /** Other error */
    unknown = LIBUSB_ERROR_OTHER,

};
struct error_category_t : public std::error_category
{
    const char *name () const noexcept override
    {
        return "co_usb usb error";
    }

    std::string message (int v) const override
    {
        return libusb_strerror(v);
    }
};

inline const std::error_category &usb_error_category ()
{
    static error_category_t instance;
    return instance;
}

inline std::error_code make_usb_error_code (usb_error e) noexcept
{
    return {static_cast<int>(e), usb_error_category()};
}

inline std::error_code make_usb_error_code (libusb_error e) noexcept
{
    return {static_cast<int>(e), usb_error_category()};
}

} // namespace co_usb

namespace std
{

template <> struct is_error_code_enum<co_usb::usb_error> : ::std::true_type
{
};

} // namespace std
