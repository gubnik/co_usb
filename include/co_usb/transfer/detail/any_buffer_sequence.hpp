#pragma once

#include <boost/capy/buffers.hpp>
namespace co_usb::transfer::detail
{

template <typename Ty>
concept AnyBufferSequence =
    boost::capy::ConstBufferSequence<Ty> || boost::capy::MutableBufferSequence<Ty>;

}
