#pragma once

#include <type_traits>

namespace co_usb::detail
{

template <typename E, typename Tag> struct flag_type
{
    using storage_t = std::underlying_type_t<E>;
    storage_t bits{0};

    constexpr flag_type() = default;
    constexpr explicit flag_type (storage_t b) : bits(b)
    {
    }

    friend constexpr flag_type operator~(flag_type f) noexcept
    {
        return flag_type{static_cast<storage_t>(~f.bits)};
    }

    friend constexpr flag_type operator&(flag_type a, flag_type b) noexcept
    {
        return flag_type{static_cast<storage_t>(a.bits & b.bits)};
    }
    friend constexpr flag_type operator|(flag_type a, flag_type b) noexcept
    {
        return flag_type{static_cast<storage_t>(a.bits | b.bits)};
    }
    friend constexpr flag_type operator^(flag_type a, flag_type b) noexcept
    {
        return flag_type{static_cast<storage_t>(a.bits ^ b.bits)};
    }

    constexpr flag_type &operator&=(flag_type other) noexcept
    {
        bits &= other.bits;
        return *this;
    }
    constexpr flag_type &operator|=(flag_type other) noexcept
    {
        bits |= other.bits;
        return *this;
    }
    constexpr flag_type &operator^=(flag_type other) noexcept
    {
        bits ^= other.bits;
        return *this;
    }

    friend constexpr bool operator==(flag_type a, flag_type b) noexcept
    {
        return a.bits == b.bits;
    }
    friend constexpr bool operator!=(flag_type a, flag_type b) noexcept
    {
        return a.bits != b.bits;
    }
};

} // namespace co_usb::detail
