#pragma once

#include <co_usb.hpp>
#include <cstring>
#include <type_traits>

template <typename T>
    requires(!std::is_pointer_v<T>)
constexpr auto const &mock ()
{
    static char storage[sizeof(T)];
    std::memset(storage, 1, sizeof(storage));
    return *reinterpret_cast<T *>(storage);
}
