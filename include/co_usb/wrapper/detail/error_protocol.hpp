#pragma once

#include <boost/capy/io_task.hpp>
#include <concepts>
#include <optional>
#include <system_error>
#include <type_traits>
#include <utility>

#include <version>

#ifdef __cpp_lib_expected
#include <expected>
#endif // __cpp_lib_expected

namespace co_usb::detail
{

/**
 * @ingroup wrapper
 *
 * @concept ErrorProtocol
 * @tparam Ty type of an error protocol
 * @tparam ValTy type of result value
 *
 * @brief Result protocol for signaling error via error code or full completion.
 *
 * @details Uniform protocol that allows the compile-time selection of an error protocol
 * for a wrapper function when a partial result is disallowed.
 *
 * @par Guarantees
 * Error protocol is expected to be transient and be move constructible and assignable.
 * Its destructor is expected to not invoke any meaningful logic.
 *
 * `with_success` signals complete success and returns a successful result value.
 * `with_error` signals error and returns an erronous result value.
 */
template <typename Ty, typename ValTy>
concept ErrorProtocol = requires(Ty errp) {
    typename Ty::template return_type<ValTy>;
    {
        errp.template with_error<ValTy>(std::declval<std::error_code>())
    } -> std::same_as<typename Ty::template return_type<ValTy>>;
    {
        errp.template with_success<ValTy>(std::declval<ValTy>())
    } -> std::same_as<typename Ty::template return_type<ValTy>>;
};

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

struct as_io_result_t
{
    template <typename ValTy> using return_type = boost::capy::io_result<std::optional<ValTy>>;

    constexpr explicit as_io_result_t () noexcept
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
static_assert(ErrorProtocol<as_io_result_t, int>, "Not an error protocol");

struct as_io_task_t
{
    template <typename ValTy> using return_type = boost::capy::io_task<std::optional<ValTy>>;

    constexpr explicit as_io_task_t () noexcept
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
static_assert(ErrorProtocol<as_io_task_t, int>, "Not an error protocol");

} // namespace co_usb::detail
