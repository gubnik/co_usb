#pragma once

#include "co_usb/wrapper/detail/error_protocol.hpp"
#include <system_error>

namespace co_usb
{
constexpr auto as_exception () noexcept -> detail::as_exception_t
{
    return detail::as_exception_t{};
}

constexpr auto as_optional (std::error_code &ec) noexcept -> detail::as_optional_t
{
    return detail::as_optional_t{ec};
}

#ifdef __cpp_lib_expected
constexpr auto as_expected () noexcept -> detail::as_expected_t
{
    return detail::as_expected_t{};
}
#endif // __cpp_lib_expected

constexpr auto as_io_result () noexcept -> detail::as_io_result_t
{
    return detail::as_io_result_t{};
}

constexpr auto as_io_task () noexcept -> detail::as_io_task_t
{
    return detail::as_io_task_t{};
}

} // namespace co_usb
