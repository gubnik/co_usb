#pragma once

#include "co_usb/transfer/detail/any_buffer_sequence.hpp"
#include "co_usb/transfer/detail/transfer_sequence.hpp"
#include "co_usb/transfer/sequence_view.hpp"
#include "co_usb/transfer/status.hpp"
#include "co_usb/usb_error.hpp"
#include <boost/capy/buffers.hpp>
#include <boost/capy/concept/io_awaitable.hpp>
#include <boost/capy/continuation.hpp>
#include <boost/capy/ex/io_env.hpp>
#include <boost/capy/io_result.hpp>
#include <cassert>
#include <coroutine>
#include <libusb.h>
#include <mutex>
#include <utility>

namespace co_usb::transfer::detail
{

template <detail::TransferSequence TSeq, AnyBufferSequence BuffersTy> struct sequence_awaitable
{
    using tfer_iter_t = sequence_view<TSeq>::iterator_type;
    using buf_iter_t = decltype(boost::capy::begin(std::declval<BuffersTy const &>()));
    using buffer_t = boost::capy::buffer_type<BuffersTy>;

    struct await_state_t
    {
        std::mutex mutex;

        boost::capy::io_env const *io_env;
        boost::capy::continuation cont;

        std::error_code ec;
        size_t total{0};
        size_t in_flight{0};
    };

    static void transfer_callback (libusb_transfer *tfer) noexcept
    {
        using self_t = sequence_awaitable<TSeq, BuffersTy>;
        self_t &self = *static_cast<self_t *>(tfer->user_data);
        const auto resume_on_zero = [&] ()
        {
            if (self.await_state->in_flight == 0)
            {
                self.await_state->io_env->executor.post(self.await_state->cont);
            }
        };
        self.await_state->in_flight--;
        bool ok = false;
        {
            std::unique_lock lock{self.await_state->mutex};
            ok = (bool)self.await_state->ec;
        }
        if (ok)
        {
            resume_on_zero();
            return;
        }
        if (tfer->type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)
        {
            for (int i = 0; i < tfer->num_iso_packets; i++)
            {
                self.await_state->total += tfer->iso_packet_desc[i].actual_length;
                if (tfer->status != LIBUSB_TRANSFER_COMPLETED) [[unlikely]]
                {
                    {
                        std::unique_lock lock{self.await_state->mutex};
                        self.await_state->ec = make_transfer_status(
                            static_cast<status>(tfer->iso_packet_desc[i].status));
                    }
                    resume_on_zero();
                    return;
                }
            }
        }
        else
        {
            self.await_state->total += tfer->actual_length;
        }
        if (tfer->status != LIBUSB_TRANSFER_COMPLETED) [[unlikely]]
        {
            {
                std::unique_lock lock{self.await_state->mutex};
                self.await_state->ec = make_transfer_status(static_cast<status>(tfer->status));
            }
            resume_on_zero();
            return;
        }
        if (self.buf_current == self.buf_end) [[unlikely]]
        {
            resume_on_zero();
            return;
        }
        buffer_t buf = *self.buf_current;
        self.buf_current++;
        tfer->buffer = (unsigned char *)buf.data();
        tfer->length = buf.size();
        int r = libusb_submit_transfer(tfer);
        if (r != LIBUSB_SUCCESS) [[unlikely]]
        {
            {
                std::unique_lock lock{self.await_state->mutex};
                self.await_state->ec = make_usb_error_code(static_cast<usb_error>(r));
            }
            resume_on_zero();
            return;
        }
        self.await_state->in_flight++;
    }

    explicit inline sequence_awaitable (await_state_t *await_state, sequence_view<TSeq> *seq_view,
                                        BuffersTy const &buffers, size_t submission_size)
        : await_state(await_state), view(seq_view), buf_current(boost::capy::begin(buffers)),
          buf_end(boost::capy::end(buffers)), submission_size(submission_size)
    {
        assert((submission_size <= seq_view->size()) &&
               "submission size must not be greater than number of transfers");
        assert((submission_size <= boost::capy::buffer_length(buffers)) &&
               "submission size must not be greater than number of buffers");
    }

    inline bool await_ready ()
    {
        return buf_current >= buf_end;
    }

    inline std::coroutine_handle<> await_suspend (std::coroutine_handle<> h,
                                                  boost::capy::io_env const *io_env)
    {
        if (io_env->stop_token.stop_requested())
        {
            await_state->ec = make_transfer_status(status::cancelled);
            return h;
        }
        await_state->io_env = io_env;
        await_state->cont = {h};
        sequence_view<TSeq> &v = *view;
        const tfer_iter_t end = v.end() + submission_size;
        for (tfer_iter_t iter = v.begin(); iter != end; iter++)
        {
            libusb_transfer *tfer = transfer_of(*iter);
            buffer_t buf = *buf_current;
            buf_current++;
            tfer->buffer = (unsigned char *)buf.data();
            tfer->length = buf.size();
            tfer->user_data = this;
            tfer->callback = transfer_callback;
        }
        await_state->in_flight = submission_size;
        for (tfer_iter_t iter = v.begin(); iter != end; iter++)
        {
            libusb_transfer *tfer = transfer_of(*iter);
            int r = libusb_submit_transfer(tfer);
            if (r != LIBUSB_SUCCESS) [[unlikely]]
            {
                std::unique_lock lock{await_state->mutex};
                await_state->ec = make_transfer_status(static_cast<status>(r));
                lock.unlock();
                break;
            }
        }
        return std::noop_coroutine();
    }

    inline boost::capy::io_result<size_t> await_resume ()
    {
        return {await_state->ec, await_state->total};
    }

    await_state_t *await_state;
    sequence_view<TSeq> *view;

    buf_iter_t buf_current;
    buf_iter_t buf_end;

    size_t submission_size{0};
};

static_assert(
    boost::capy::IoAwaitable<sequence_awaitable<libusb_transfer *, boost::capy::mutable_buffer>>,
    "Not a proper IoAwaitable");
static_assert(boost::capy::IoAwaitable<
                  sequence_awaitable<std::vector<libusb_transfer *>, boost::capy::const_buffer>>,
              "Not a proper IoAwaitable");

} // namespace co_usb::transfer::detail
