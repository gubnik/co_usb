/**
 * @file any_buffer_sequence.hpp
 * @brief Unified concept for buffer sequences.
 */

#pragma once

#include <boost/capy/buffers.hpp>

namespace co_usb::transfer::detail
{

/**
 * @ingroup transfer
 *
 * @concept AnyBufferSequence
 *
 * @brief Unified concept for buffer sequences.
 */
template <typename Ty>
concept AnyBufferSequence =
    boost::capy::ConstBufferSequence<Ty> || boost::capy::MutableBufferSequence<Ty>;

} // namespace co_usb::transfer::detail
