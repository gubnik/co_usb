#pragma once

#include "co_usb/transfer/detail/iterator.hpp"
#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include <libusb.h>
#include <utility>
#include <vector>

namespace co_usb::transfer
{

/**
 * @ingroup transfer
 *
 * @brief View of a certain transfer sequence. Provides a uniform range interface.
 *
 * @details Use this adaptor for uniform handling of transfer sequences consisting of a singular
 * transfer resource and ranges of transfer resources.
 *
 * @note Cannot bind to a temporary view.
 */
template <detail::TransferSequence Seq> struct transfer_sequence_view
{
    using iterator_type = decltype(detail::transfer_begin(std::declval<Seq const &>()));

    transfer_sequence_view (Seq const &seq) noexcept
        : m_begin(detail::transfer_begin(seq)), m_end(detail::transfer_end(seq))
    {
    }

    transfer_sequence_view (iterator_type begin, iterator_type end) noexcept
        : m_begin(begin), m_end(end)
    {
    }

    // disallow bind to temporary view
    transfer_sequence_view(Seq const &&) = delete;

    auto size () const noexcept -> size_t
    {
        return m_end - m_begin;
    }

    [[nodiscard]] auto begin () const noexcept
    {
        return m_begin;
    }

    [[nodiscard]] auto end () const noexcept
    {
        return m_begin;
    }

  private:
    iterator_type m_begin;
    iterator_type m_end;
};

template <class Seq> transfer_sequence_view(Seq &) -> transfer_sequence_view<Seq>;
static_assert(detail::TransferSequence<transfer_sequence_view<libusb_transfer *>>,
              "Not a proper transfer sequence");
static_assert(detail::TransferSequence<transfer_sequence_view<std::vector<libusb_transfer *>>>,
              "Not a proper transfer sequence");

} // namespace co_usb::transfer
