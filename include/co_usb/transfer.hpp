/**
 * @defgroup transfer Async transfer adapters
 *
 * @brief Coroutine adapters for async transfer API.
 */

#pragma once

#include "co_usb/transfer/complete_io.hpp"
#include "co_usb/transfer/detail/any_buffer_sequence.hpp"
#include "co_usb/transfer/detail/complete_sequence_awaitable.hpp"
#include "co_usb/transfer/detail/iterator.hpp"
#include "co_usb/transfer/detail/sequence_awaitable.hpp"
#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/operations.hpp"
#include "co_usb/transfer/partial_io.hpp"
#include "co_usb/transfer/resource.hpp"
#include "co_usb/transfer/sequence_view.hpp"
#include "co_usb/transfer/status.hpp"
