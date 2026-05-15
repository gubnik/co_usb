// clang-format off
#include <boost/capy/concept/read_stream.hpp>
#include <boost/capy/io/any_read_stream.hpp>
#include <boost/capy/io/any_write_stream.hpp>
#include <boost/capy/concept/write_stream.hpp>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <co_usb.hpp>
#include "co_usb/transfer/endpoint.hpp"
#include "co_usb/transfer/transfer_types.hpp"
#include "test_mock.hpp"
// clang-format on

static co_usb::interface const &iface = mock<co_usb::interface>(); // dummy

TEST_CASE("transfer-sfinae", "[transfer]")
{
    // bulk, interrupt, iso and bulk stream transfers must have read for IN and write for OUT
    // endpoints, i.e. satisfy ReadStream and WriteStream concepts
    REQUIRE(boost::capy::ReadStream<co_usb::bulk_transfer<co_usb::ep_direction::in>> == true);
    REQUIRE(boost::capy::WriteStream<co_usb::bulk_transfer<co_usb::ep_direction::in>> == false);
    REQUIRE(boost::capy::ReadStream<co_usb::bulk_transfer<co_usb::ep_direction::out>> == false);
    REQUIRE(boost::capy::WriteStream<co_usb::bulk_transfer<co_usb::ep_direction::out>> == true);

    // a control transfer is bidirectional and must have both read and write awailable
    REQUIRE(boost::capy::ReadStream<co_usb::control_transfer> == true);
    REQUIRE(boost::capy::WriteStream<co_usb::control_transfer> == true);
}

TEST_CASE("transfer-buffers", "[transfer][buffers]")
{
    REQUIRE(requires(co_usb::bulk_transfer<co_usb::ep_direction::in> &tfer,
                     boost::capy::mutable_buffer buffer,
                     std::span<boost::capy::mutable_buffer> buffers) {
        { tfer.read_some(buffer) };
        { tfer.read_some(buffers) };
    });
    REQUIRE(requires(co_usb::bulk_transfer<co_usb::ep_direction::out> &tfer,
                     boost::capy::const_buffer buffer,
                     std::span<boost::capy::const_buffer> buffers) {
        { tfer.write_some(buffer) };
        { tfer.write_some(buffers) };
    });
}

TEST_CASE("any-transfer", "[transfers]")
{
    REQUIRE(
        [&]
        {
            auto tfer = co_usb::bulk_transfer(co_usb::ep_in(0x81, iface));
            boost::capy::any_read_stream read_s{&tfer};
            return true;
        }());
    REQUIRE(
        [&]
        {
            auto tfer = co_usb::bulk_transfer(co_usb::ep_out(0x01, iface));
            boost::capy::any_write_stream write_s{&tfer};
            return true;
        }());
}
