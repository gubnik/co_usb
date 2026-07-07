#pragma once

#include <boost/capy/io_task.hpp>
#include <concepts>
#include <optional>
#include <system_error>
#include <utility>

#include <version>

#ifdef __cpp_lib_expected
#include <expected>
#endif // __cpp_lib_expected

namespace co_usb::detail
{

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

struct as_exception_t
{
    template <typename ValTy> using return_type = ValTy;

    explicit as_exception_t ()
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

struct as_optional_t
{
    template <typename ValTy> using return_type = std::optional<ValTy>;

    explicit as_optional_t (std::error_code &ec) noexcept : m_ec_ref(ec)
    {
    }

    template <typename U> auto with_error (std::error_code ec) noexcept -> return_type<U>
    {
        m_ec_ref = ec;
        return std::nullopt;
    }

    template <typename U, typename... Args> auto with_success (Args &&...args) -> return_type<U>
    {
        m_ec_ref.clear();
        return std::optional<U>(std::in_place, std::forward<Args>(args)...);
    }

  private:
    std::error_code &m_ec_ref;
};
static_assert(ErrorProtocol<as_optional_t, int>, "Not an error protocol");

#ifdef __cpp_lib_expected
struct as_expected_t
{
    template <typename ValTy> using return_type = std::expected<ValTy, std::error_code>;

    explicit as_expected_t () noexcept
    {
    }

    template <typename U> auto with_error (std::error_code ec) noexcept -> return_type<U>
    {
        return std::unexpected(ec);
    }

    template <typename U, typename... Args> auto with_success (Args &&...args) -> return_type<U>
    {
        return return_type<U>(std::in_place, std::forward<Args>(args)...);
    }
};
static_assert(ErrorProtocol<as_expected_t, int>, "Not an error protocol");
#endif // __cpp_lib_expected

struct as_io_result_t
{
    template <typename ValTy> using return_type = boost::capy::io_result<std::optional<ValTy>>;

    explicit as_io_result_t () noexcept
    {
    }

    template <typename U> auto with_error (std::error_code ec) noexcept -> return_type<U>
    {
        return {ec, std::nullopt};
    }

    template <typename U, typename... Args> auto with_success (Args &&...args) -> return_type<U>
    {
        return {{}, std::optional<U>(std::in_place, std::forward<Args>(args)...)};
    }
};
static_assert(ErrorProtocol<as_io_result_t, int>, "Not an error protocol");

struct as_io_task_t
{
    template <typename ValTy> using return_type = boost::capy::io_task<std::optional<ValTy>>;

    explicit as_io_task_t () noexcept
    {
    }

    template <typename U> auto with_error (std::error_code ec) noexcept -> return_type<U>
    {
        co_return {ec, std::nullopt};
    }

    template <typename U, typename... Args> auto with_success (Args &&...args) -> return_type<U>
    {
        co_return {{}, std::optional<U>(std::in_place, std::forward<Args>(args)...)};
    }
};
static_assert(ErrorProtocol<as_io_task_t, int>, "Not an error protocol");

} // namespace co_usb::detail
