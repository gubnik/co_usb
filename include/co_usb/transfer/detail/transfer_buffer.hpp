#pragma once

#include "co_usb/transfer/endpoint.hpp"
#include <boost/capy/buffers.hpp>

namespace co_usb::detail
{

template <typename AB, endpoint_direction Direction>
concept transfer_buffer =
    (boost::capy::ConstBufferSequence<AB> || boost::capy::MutableBufferSequence<AB>) &&
    (Direction == endpoint_direction::any ||
     (boost::capy::ConstBufferSequence<AB> && Direction == endpoint_direction::out) ||
     (boost::capy::MutableBufferSequence<AB> && Direction == endpoint_direction::in));

}
