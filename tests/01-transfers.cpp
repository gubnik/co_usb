// clang-format off
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <co_usb.hpp>
#include "co_usb/transfer/endpoint.hpp"
#include "test_mock.hpp"
// clang-format on

static co_usb::interface const &iface = mock<co_usb::interface>(); // dummy

template <typename T>
constexpr bool has_write_some_v =
    requires { std::declval<T &>().write_some(std::declval<boost::capy::const_buffer>()); };

template <typename T>
constexpr bool has_read_some_v =
    requires { std::declval<T &>().read_some(std::declval<boost::capy::mutable_buffer>()); };

TEST_CASE("transfer-sfinae", "[transfer]")
{
    // bulk, interrupt, iso and bulk stream transfers must have read for IN and write for OUT
    // endpoints
    REQUIRE(has_read_some_v<co_usb::bulk_transfer<co_usb::ep_direction::in>> == true);
    REQUIRE(has_write_some_v<co_usb::bulk_transfer<co_usb::ep_direction::in>> == false);
    REQUIRE(has_read_some_v<co_usb::bulk_transfer<co_usb::ep_direction::out>> == false);
    REQUIRE(has_write_some_v<co_usb::bulk_transfer<co_usb::ep_direction::out>> == true);

    // a control transfer is bidirectional and must have both read and write awailable
    REQUIRE(has_read_some_v<co_usb::control_transfer> == true);
    REQUIRE(has_write_some_v<co_usb::control_transfer> == true);
}
