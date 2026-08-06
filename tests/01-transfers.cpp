// clang-format off
#include <boost/capy.hpp>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <co_usb.hpp>
#include <libusb.h>
#include <vector>
#include "test_mock.hpp"
// clang-format on

struct devh_provider
{
    auto devh () const noexcept -> libusb_device_handle *
    {
        return mock<libusb_device_handle *>(); // dummy
    }
};

static devh_provider iface{};
static_assert(co_usb::detail::DeviceHandleSource<devh_provider>, "Not a Device Source");

using tfer_seq_t = std::vector<co_usb::transfer::resource>;

TEST_CASE("transfer-sfinae", "[transfer]")
{
    using tfer_seq_t = std::vector<co_usb::transfer::resource>;
    // bulk, interrupt, iso and bulk stream transfers must have read for IN and write for OUT
    // endpoints, i.e. satisfy ReadStream and WriteStream concepts
    REQUIRE(boost::capy::ReadStream<co_usb::transfer::bulk_in<tfer_seq_t>> == true);
    REQUIRE(boost::capy::WriteStream<co_usb::transfer::bulk_in<tfer_seq_t>> == false);
    REQUIRE(boost::capy::ReadStream<co_usb::transfer::bulk_out<tfer_seq_t>> == false);
    REQUIRE(boost::capy::WriteStream<co_usb::transfer::bulk_out<tfer_seq_t>> == true);
}

TEST_CASE("transfer-buffers", "[transfer][buffers]")
{
    REQUIRE(requires(co_usb::transfer::bulk_in<tfer_seq_t> &tfer,
                     boost::capy::mutable_buffer buffer,
                     std::span<boost::capy::mutable_buffer> buffers) {
        { tfer.read_some(buffer) };
        { tfer.read_some(buffers) };
    });
    REQUIRE(requires(co_usb::transfer::bulk_out<tfer_seq_t> &tfer, boost::capy::const_buffer buffer,
                     std::span<boost::capy::const_buffer> buffers) {
        { tfer.write_some(buffer) };
        { tfer.write_some(buffers) };
    });
}

TEST_CASE("any-transfer", "[transfers]")
{
    REQUIRE(
        [&] () -> boost::capy::task<>
                  {
                      tfer_seq_t tfer_seq{};
                      auto exec = co_await boost::capy::this_coro::executor;
                      co_usb::transfer::bulk_in tfer(exec, tfer_seq,
                                                     co_usb::transfer::ep_in(0x81, iface));
                      boost::capy::any_read_stream read_s{&tfer};
                  }()
                      .handle());
    REQUIRE(
        [&] () -> boost::capy::task<>
                  {
                      tfer_seq_t tfer_seq{};
                      auto exec = co_await boost::capy::this_coro::executor;
                      co_usb::transfer::bulk_out tfer(exec, tfer_seq,
                                                      co_usb::transfer::ep_out(0x01, iface));
                      boost::capy::any_write_stream write_s{&tfer};
                  }()
                      .handle());
}

TEST_CASE("transfer-operations", "[transfer]")
{
    libusb_device_handle *ndevh = nullptr;
    REQUIRE(
        [=] () -> bool
        {
            auto ep_out =
                co_usb::transfer::endpoint<co_usb::transfer::endpoint_direction::out>::make_unsafe(
                    0x01, ndevh);
            co_usb::transfer::resource tfer;
            co_usb::transfer::prefill_bulk(tfer, ep_out);
            return tfer.get()->endpoint == 0x01 &&
                   tfer.get()->type == LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK;
        }());
    REQUIRE(
        [=] () -> bool
        {
            auto ep_out_1 =
                co_usb::transfer::endpoint<co_usb::transfer::endpoint_direction::out>::make_unsafe(
                    0x01, ndevh);
            auto ep_out_2 =
                co_usb::transfer::endpoint<co_usb::transfer::endpoint_direction::out>::make_unsafe(
                    0x02, ndevh);
            std::vector<co_usb::transfer::resource> tfers;
            tfers.resize(16);
            co_usb::transfer::prefill_bulk(tfers, ep_out_1);
            for (auto const &tfer_res : tfers)
            {
                if (tfer_res.get()->endpoint == 0x01 &&
                    tfer_res.get()->type == LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK)
                    continue;
                return false;
            }
            co_usb::transfer::prefill_interrupt_transfer(tfers, ep_out_2);
            for (auto const &tfer_res : tfers)
            {
                if (tfer_res.get()->endpoint == 0x02 &&
                    tfer_res.get()->type == LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT)
                    continue;
                return false;
            }
            return true;
        }());
}

TEST_CASE("transfer-sequence-view", "[transfer]")
{
    REQUIRE(
        [] () -> bool
        {
            co_usb::transfer::resource tfer;
            co_usb::transfer::sequence_view pool_1{tfer};
            if (pool_1.size() != 1)
            {
                return false;
            }
            return true;
        });
    REQUIRE(
        [] () -> bool
        {
            std::vector<co_usb::transfer::resource> tfers;
            tfers.resize(16);
            co_usb::transfer::sequence_view pool_1{tfers};
            if (pool_1.size() != 16)
            {
                return false;
            }
            return true;
        });
}

TEST_CASE("transfer-stream", "[transfer]")
{
    REQUIRE(
        [] () -> boost::capy::task<>
                 {
                     auto exec = co_await boost::capy::this_coro::executor;
                     std::vector<co_usb::transfer::resource> tfers;
                     tfers.resize(16);
                     co_usb::transfer::detail::partial_io_base stream{exec, tfers};
                     char buf[1024];
                     (void)co_await stream.submit(boost::capy::make_buffer(buf));
                     co_return;
                 }()
                     .handle());
}
