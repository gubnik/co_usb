#pragma once

#include "co_usb/ev/event_handler_ref.hpp"
#include "co_usb/transfer/transfer_status.hpp"
#include "co_usb/usb_error.hpp"
#include <boost/capy/buffers.hpp>
#include <boost/capy/ex/io_env.hpp>
#include <boost/capy/io_result.hpp>
#include <coroutine>
#include <iterator>
#include <libusb.h>
#include <mutex>
#include <span>
#include <system_error>
#include <utility>

namespace co_usb::detail
{

template <typename AnyBufferSequence> struct sliding_transfer_awaitable
{
    using buffer_t = boost::capy::buffer_type<AnyBufferSequence>;
    struct await_state
    {
        await_state (AnyBufferSequence const &buffers, event_handler_ref ev_ref) noexcept
            : total_buffers(boost::capy::buffer_length(buffers)), iter(std::begin(buffers)),
              iter_end(std::end(buffers)) //, ev_ref(ev_ref)
        {
        }

        std::mutex mutex;
        std::error_code ec{};
        size_t total{0};

        const size_t total_buffers{0};
        size_t in_flight{0};

        decltype(std::begin(std::declval<AnyBufferSequence const &>())) iter;
        const decltype(iter) iter_end;

        boost::capy::io_env const *io_env = nullptr;
        boost::capy::continuation cont{};
    };

    sliding_transfer_awaitable (await_state &aw_state, std::span<libusb_transfer *> trasfers)
        : m_transfers(trasfers), m_aw_state(&aw_state)
    {
    }

    bool await_ready ()
    {
        return m_aw_state->iter == m_aw_state->iter_end;
    }

    std::coroutine_handle<> await_suspend (std::coroutine_handle<> h,
                                           boost::capy::io_env const *env)
    {
        m_aw_state->io_env = env;
        m_aw_state->cont   = boost::capy::continuation{h};

        if (env->stop_token.stop_requested())
        {
            m_aw_state->ec = make_transfer_status(transfer_status::cancelled);
            return h;
        }

        const size_t total_buffers = m_aw_state->total_buffers;
        const size_t num_to_fill   = std::min(total_buffers, m_transfers.size());

        for (size_t i = 0; i < num_to_fill; i++)
        {
            auto &tfer   = m_transfers[i];
            buffer_t buf = *(m_aw_state->iter);
            m_aw_state->iter++;
            tfer->user_data = m_aw_state;
            tfer->buffer    = (unsigned char *)buf.data();
            tfer->length    = buf.size();
            tfer->callback  = [] (libusb_transfer *transfer)
            {
                using aw_state_t = await_state;
                aw_state_t &self = *static_cast<aw_state_t *>(transfer->user_data);
                // auto ev_ref               = self.ev_ref;
                const auto resume_on_zero = [&] ()
                {
                    if (self.in_flight == 0)
                    {
                        self.io_env->executor.post(self.cont);
                    }
                };
                self.in_flight--;
                std::unique_lock lock{self.mutex};
                if (self.ec)
                {
                    lock.unlock();
                    resume_on_zero();
                    return;
                }
                self.total += (transfer->actual_length);
                lock.unlock();
                if (transfer->status != LIBUSB_TRANSFER_COMPLETED)
                {
                    lock.lock();
                    self.ec = make_transfer_status(static_cast<transfer_status>(transfer->status));
                    lock.unlock();
                    resume_on_zero();
                    return;
                }
                if (self.iter == self.iter_end)
                {
                    resume_on_zero();
                    return;
                }
                buffer_t buf = *self.iter;
                self.iter++;
                transfer->buffer = (unsigned char *)buf.data();
                transfer->length = buf.size();
                int r            = libusb_submit_transfer(transfer);
                if (r != LIBUSB_SUCCESS)
                {
                    lock.lock();
                    self.ec = make_usb_error_code(static_cast<usb_error>(r));
                    lock.unlock();
                    resume_on_zero();
                    return;
                }
                self.in_flight++;
            };
        }
        m_aw_state->in_flight = num_to_fill;
        std::unique_lock lock{m_aw_state->mutex};
        for (size_t i = 0; i < num_to_fill; i++)
        {
            auto &tfer = m_transfers[i];
            int r      = libusb_submit_transfer(tfer);
            if (r != LIBUSB_SUCCESS)
            {
                m_aw_state->ec = make_usb_error_code(static_cast<usb_error>(r));
                lock.unlock();
                break;
            }
        }

        return std::noop_coroutine();
    }

    boost::capy::io_result<size_t> await_resume ()
    {
        if (m_aw_state->ec)
        {
            return {m_aw_state->ec, m_aw_state->total};
        }
        return {std::error_code{}, m_aw_state->total};
    }

  private:
    std::span<libusb_transfer *> m_transfers;
    await_state *m_aw_state;
};

} // namespace co_usb::detail
