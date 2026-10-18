#pragma once

#include <boost/capy/results.hpp>
#include <boost/capy/task.hpp>
#include <concepts>
#include <optional>
#include <system_error>
#include <type_traits>
#include <utility>

#include <version>

#ifdef __cpp_lib_expected
#include <expected>
#endif // __cpp_lib_expected

namespace co_usb
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
} // namespace co_usb
