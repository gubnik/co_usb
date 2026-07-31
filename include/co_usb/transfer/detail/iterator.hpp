#pragma once

#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include <memory>
namespace co_usb::transfer::detail
{

constexpr struct
{

    template <detail::TransferResource TRes>
    inline auto operator()(TRes const &tfer_res) const noexcept -> TRes const *
    {
        return std::addressof(tfer_res);
    }

    template <detail::TransferSequence TSeq>
        requires(!detail::TransferResource<TSeq>)
    inline auto operator()(TSeq const &tfer_seq) const noexcept
    {
        return std::ranges::begin(tfer_seq);
    }

    template <detail::TransferSequence TSeq>
        requires(!detail::TransferResource<TSeq>)
    inline auto operator()(TSeq &tfer_seq) const noexcept
    {
        return std::ranges::begin(tfer_seq);
    }

} transfer_begin{};

constexpr struct
{
    template <detail::TransferResource TRes>
    inline auto operator()(TRes const &tfer_res) const noexcept -> TRes const *
    {
        return std::addressof(tfer_res) + 1;
    }

    template <detail::TransferSequence TSeq>
        requires(!detail::TransferResource<TSeq>)
    inline auto operator()(TSeq const &tfer_seq) const noexcept
    {
        return std::ranges::end(tfer_seq);
    }

    template <detail::TransferSequence TSeq>
        requires(!detail::TransferResource<TSeq>)
    inline auto operator()(TSeq &tfer_seq) const noexcept
    {
        return std::ranges::end(tfer_seq);
    }

} transfer_end{};

} // namespace co_usb::transfer::detail
