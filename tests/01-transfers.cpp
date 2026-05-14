// clang-format off
#include <boost/capy/concept/read_stream.hpp>
#include <boost/capy/concept/write_stream.hpp>
#include <catch2/catch_all.hpp>
#include <co_usb.hpp>
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
