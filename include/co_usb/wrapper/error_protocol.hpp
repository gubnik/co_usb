/**
 * @file error_protocol.hpp
 * @brief Utilities for creating error protocols.
 */

#pragma once

#include "co_usb/wrapper/concepts/error_protocol.hpp"
#include <system_error>

namespace co_usb
{

/**
 * @ingroup wrapper
 *
 * @brief Exception error protocol
 *
 * @details Base error protocol that signals errors as std::system_error exceptions.
 * Its result value is the success value type.
 *
 * @note `with_error` throws std::system_error.
 */
struct as_exception_t
{
    template <typename ValTy> using return_type = ValTy;

    constexpr explicit as_exception_t ()
    {
    }

    template <typename U> [[noreturn]] auto with_error (std::error_code ec) -> return_type<U>
    {
        throw std::system_error{ec};
    }

    template <typename U, typename... Args> auto with_success (Args &&...args) -> return_type<U>
    {
        return U(std::forward<Args>(args)...);
    }
};
static_assert(ErrorProtocol<as_exception_t, int>, "Not an error protocol");

/**
 * @ingroup wrapper
 *
 * @brief Error protocol signaling error into a given error code reference and returning an
 * optional.
 *
 * @details On error, assigns the given error code to a stored error code reference and returns a
 * nullopt. On success, clears error code by refernce and returns an optional with a success value.
 *
 * @note `with_error` is noexcept. `with_success` is noexcept if chosen ctor is noexcept.
 */
struct as_optional_t
{
    template <typename ValTy> using return_type = std::optional<ValTy>;

    constexpr explicit as_optional_t (std::error_code &ec) noexcept : m_ec_ref(ec)
    {
    }

    /**
     * @brief Signals error to stored error code reference.
     *
     * @returns std::nullopt.
     */
    template <typename U> auto with_error (std::error_code ec) noexcept -> return_type<U>
    {
        m_ec_ref = ec;
        return std::nullopt;
    }

    /**
     * @brief Signals success via returned optional.
     *
     * @returns std::optional with constructed result.
     */
    template <typename U, typename... Args>
    auto with_success (Args &&...args) noexcept(std::is_nothrow_constructible_v<U, Args...>)
        -> return_type<U>
    {
        m_ec_ref.clear();
        return std::optional<U>(std::in_place, std::forward<Args>(args)...);
    }

  private:
    std::error_code &m_ec_ref;
};
static_assert(ErrorProtocol<as_optional_t, int>, "Not an error protocol");

#ifdef __cpp_lib_expected
/**
 * @ingroup wrapper
 *
 * @brief Signals error via unexpected value.
 *
 * @details On error, returns std::expected with unexpected of an error code.
 * On success, returns std::expected with constructed result value.
 */
struct as_expected_t
{
    template <typename ValTy> using return_type = std::expected<ValTy, std::error_code>;

    constexpr explicit as_expected_t () noexcept
    {
    }

    /**
     * @brief Signals error.
     *
     * @returns std::unexpected of given error code.
     */
    template <typename U> auto with_error (std::error_code ec) noexcept -> return_type<U>
    {
        return std::unexpected(ec);
    }

    /**
     * @brief Signals success.
     *
     * @returns std::expected of newly constructed result value.
     */
    template <typename U, typename... Args>
    auto with_success (Args &&...args) noexcept(std::is_nothrow_constructible_v<U, Args...>)
        -> return_type<U>
    {
        return return_type<U>(std::in_place, std::forward<Args>(args)...);
    }
};
static_assert(ErrorProtocol<as_expected_t, int>, "Not an error protocol");
#endif // __cpp_lib_expected

struct as_results_t
{
    template <typename ValTy> using return_type = boost::capy::results<std::optional<ValTy>>;

    constexpr explicit as_results_t () noexcept
    {
    }

    template <typename U> auto with_error (std::error_code ec) noexcept -> return_type<U>
    {
        return {ec, std::nullopt};
    }

    template <typename U, typename... Args>
    auto with_success (Args &&...args) noexcept(std::is_nothrow_constructible_v<U, Args...>)
        -> return_type<U>
    {
        return {{}, std::optional<U>(std::in_place, std::forward<Args>(args)...)};
    }
};
static_assert(ErrorProtocol<as_results_t, int>, "Not an error protocol");

struct as_task_t
{
    template <typename ValTy>
    using return_type = boost::capy::task<boost::capy::results<std::optional<ValTy>>>;

    constexpr explicit as_task_t () noexcept
    {
    }

    template <typename U> auto with_error (std::error_code ec) noexcept -> return_type<U>
    {
        co_return {ec, std::nullopt};
    }

    template <typename U, typename... Args>
    auto with_success (Args &&...args) noexcept(std::is_nothrow_constructible_v<U, Args...>)
        -> return_type<U>
    {
        co_return {{}, std::optional<U>(std::in_place, std::forward<Args>(args)...)};
    }
};
static_assert(ErrorProtocol<as_task_t, int>, "Not an error protocol");

constexpr auto as_exception () noexcept -> as_exception_t
{
    return as_exception_t{};
}

constexpr auto as_optional (std::error_code &ec) noexcept -> as_optional_t
{
    return as_optional_t{ec};
}

#ifdef __cpp_lib_expected
constexpr auto as_expected () noexcept -> as_expected_t
{
    return as_expected_t{};
}
#endif // __cpp_lib_expected

constexpr auto as_results () noexcept -> as_results_t
{
    return as_results_t{};
}

constexpr auto as_task () noexcept -> as_task_t
{
    return as_task_t{};
}

} // namespace co_usb
