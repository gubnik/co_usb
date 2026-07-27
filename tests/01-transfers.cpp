// clang-format off
#include <boost/capy/concept/read_stream.hpp>
#include <boost/capy/ex/this_coro.hpp>
#include <boost/capy/io/any_read_stream.hpp>
#include <boost/capy/io/any_write_stream.hpp>
#include <boost/capy/concept/write_stream.hpp>
#include <boost/capy/task.hpp>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <co_usb/co_usb.hpp>
#include <libusb.h>
#include <optional>
#include <vector>
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/transfer_pool.hpp"
#include "co_usb/transfer/transfer_types.hpp"
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

TEST_CASE("transfer-sfinae", "[transfer]")
{
    // bulk, interrupt, iso and bulk stream transfers must have read for IN and write for OUT
    // endpoints, i.e. satisfy ReadStream and WriteStream concepts
    REQUIRE(boost::capy::ReadStream<co_usb::bulk_transfer<co_usb::endpoint_direction::in>> == true);
    REQUIRE(boost::capy::WriteStream<co_usb::bulk_transfer<co_usb::endpoint_direction::in>> ==
            false);
    REQUIRE(boost::capy::ReadStream<co_usb::bulk_transfer<co_usb::endpoint_direction::out>> ==
            false);
    REQUIRE(boost::capy::WriteStream<co_usb::bulk_transfer<co_usb::endpoint_direction::out>> ==
            true);

    // a control transfer is bidirectional and must have both read and write awailable
    REQUIRE(boost::capy::ReadStream<co_usb::control_transfer<>> == true);
    REQUIRE(boost::capy::WriteStream<co_usb::control_transfer<>> == true);
}

TEST_CASE("transfer-buffers", "[transfer][buffers]")
{
    REQUIRE(requires(co_usb::bulk_transfer<co_usb::endpoint_direction::in> &tfer,
                     boost::capy::mutable_buffer buffer,
                     std::span<boost::capy::mutable_buffer> buffers) {
        { tfer.read_some(buffer) };
        { tfer.read_some(buffers) };
    });
    REQUIRE(requires(co_usb::bulk_transfer<co_usb::endpoint_direction::out> &tfer,
                     boost::capy::const_buffer buffer,
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
                      auto exec = co_await boost::capy::this_coro::executor;
                      auto tfer = co_usb::bulk_transfer(exec, co_usb::endpoint_in(0x81, iface));
                      boost::capy::any_read_stream read_s{&tfer};
                  }()
                      .handle());
    REQUIRE(
        [&] () -> boost::capy::task<>
                  {
                      auto exec = co_await boost::capy::this_coro::executor;
                      auto tfer = co_usb::bulk_transfer(exec, co_usb::endpoint_out(0x01, iface));
                      boost::capy::any_write_stream write_s{&tfer};
                  }()
                      .handle());
}

TEST_CASE("transfer-pool", "[transfers]")
{
    REQUIRE(
        [&] () -> bool
        {
            std::vector<libusb_transfer *> tfers;
            tfers.resize(16);
            co_usb::transfer_pool pool(tfers);
            libusb_transfer *tfer{nullptr};
            {
                std::optional<size_t> maybe_idx = pool.acquire(tfer);
                if (!maybe_idx.has_value() || *maybe_idx != 0)
                {
                    return false;
                }
                pool.release(*maybe_idx);
            }
            {
                std::optional<size_t> maybe_idx = pool.acquire(tfer);
                if (!maybe_idx.has_value() || *maybe_idx != 0)
                {
                    return false;
                }
                maybe_idx = pool.acquire(tfer);
                if (!maybe_idx.has_value() || *maybe_idx != 1)
                {
                    return false;
                }
                pool.release(*maybe_idx);
            }
            return true;
        }());
}
