#pragma once

#include <libusb-1.0/libusb.h>
#include <system_error>
#include <type_traits>
namespace co_usb
{

/**
 * @brief status codes for transfers
 *
 * @note can be casted from @ref libusb_transfer_status
 */
enum class transfer_status
{
    completed = LIBUSB_TRANSFER_COMPLETED,

    /** Transfer failed */
    error = LIBUSB_TRANSFER_ERROR,

    /** Transfer timed out */
    cancelled = LIBUSB_TRANSFER_CANCELLED,

    /** For bulk/interrupt endpoints: halt condition detected (endpoint
     * stalled). For control endpoints: control request not supported. */
    stall = LIBUSB_TRANSFER_STALL,

    /** Device was disconnected */
    no_device = LIBUSB_TRANSFER_NO_DEVICE,

    /** Device sent more data than requested */
    overflow = LIBUSB_TRANSFER_OVERFLOW,
};

struct transfer_status_category_t : public std::error_category
{
    const char *name () const noexcept override
    {
        return "co_usb transfer status";
    }

    std::string message (int v) const override
    {
        switch (static_cast<transfer_status>(v))
        {
            using enum transfer_status;
        case completed: return "completed";
        case error: return "unknown error";
        case cancelled: return "cancelled";
        case stall: return "device stalled";
        case no_device: return "no such device";
        case overflow: return "data overflow";
        }
    }
};

inline const std::error_category &transfer_status_category ()
{
    static transfer_status_category_t instance;
    return instance;
}

inline std::error_code make_transfer_status (transfer_status e) noexcept
{
    return {static_cast<int>(e), transfer_status_category()};
}

inline std::error_code make_transfer_status (libusb_transfer_status e) noexcept
{
    return {static_cast<int>(e), transfer_status_category()};
}

} // namespace co_usb

namespace std
{

template <> struct is_error_code_enum<co_usb::transfer_status> : ::std::true_type
{
};

} // namespace std
