#pragma once

#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include <algorithm>
#include <atomic>
#include <iterator>
#include <libusb.h>
#include <memory_resource>
#include <optional>
#include <utility>

namespace co_usb
{
template <detail::TransferSequence Seq> struct transfer_pool
{
    using iterator_type = decltype(std::begin(std::declval<Seq const &>()));

    transfer_pool (Seq const &seq,
                   std::pmr::memory_resource *memres = std::pmr::get_default_resource())
        : m_memres(memres), m_slots(std::size(seq), memres), m_begin(std::begin(seq)),
          m_end(std::end(seq)), m_size(std::size(seq))
    {
    }

    transfer_pool(Seq const &&) = delete;

    auto empty () const noexcept -> bool
    {
        return std::empty(m_begin, get_free_slot());
    }

    [[nodiscard]] auto acquire (libusb_transfer *&out_tfer) -> std::optional<size_t>
    {
        auto slot_iter = get_free_slot();
        if (slot_iter == m_slots.end())
        {
            return std::nullopt;
        }
        size_t idx = std::distance(m_slots.begin(), slot_iter);
        if (idx >= m_size)
        {
            return std::nullopt;
        }
        iterator_type iter = m_begin;
        std::advance(iter, idx);
        libusb_transfer *tfer = transfer_of(*iter);
        slot_iter->val.store(true, std::memory_order_release);
        out_tfer = tfer;
        return std::optional<size_t>(std::in_place, idx);
    }

    auto release (size_t idx) -> void
    {
        m_slots[idx].val.store(false, std::memory_order_release);
    }

  private:
    inline auto get_free_slot ()
    {
        return std::find_if(m_slots.begin(), m_slots.end(), [] (slot_t const &slot)
                            { return slot.val.load(std::memory_order_acquire) == false; });
    }

  private:
    struct slot_t
    {
        slot_t () noexcept : val(false)
        {
        }

        std::atomic_bool val{false};
    };

    std::pmr::memory_resource *m_memres;
    std::pmr::vector<slot_t> m_slots;
    iterator_type m_begin;
    iterator_type m_end;
    size_t m_size;
};

template <class Seq> transfer_pool(Seq const &) -> transfer_pool<Seq>;

} // namespace co_usb
