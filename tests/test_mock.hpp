#pragma once

#include <co_usb.hpp>
#include <type_traits>

/**
 * @brief Rough mocking function
 *
 * It is, of course, UB to use but I don't care
 */
template <typename T> constexpr auto &mock ()
{
    using type = std::remove_cvref_t<T>;
    static char storage[sizeof(type)];
    return *reinterpret_cast<type *>(storage);
}
