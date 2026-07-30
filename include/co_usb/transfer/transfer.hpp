/**
 * @defgroup transfer
 */

#pragma once

#include "co_usb/transfer/complete_io.hpp"
#include "co_usb/transfer/detail/any_buffer_sequence.hpp"
#include "co_usb/transfer/detail/complete_sequence_awaitable.hpp"
#include "co_usb/transfer/detail/device_handle_source.hpp"
#include "co_usb/transfer/detail/iterator.hpp"
#include "co_usb/transfer/detail/sequence_awaitable.hpp"
#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/stream.hpp"
#include "co_usb/transfer/transfer_operations.hpp"
#include "co_usb/transfer/transfer_resource.hpp"
#include "co_usb/transfer/transfer_sequence_view.hpp"
#include "co_usb/transfer/transfer_status.hpp"
