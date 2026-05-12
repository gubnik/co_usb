// clang-format off
#include <catch2/catch_all.hpp>
#include <co_usb.hpp>
#include "test_mock.hpp"
// clang-format on

static co_usb::interface const &iface = mock<co_usb::interface>(); // dummy

TEST_CASE("ep-conversion", "[endpoint]")
{
    REQUIRE(co_usb::ep_any(0x81, iface).as<co_usb::ep_direction::in>() != std::nullopt);
    REQUIRE(co_usb::ep_any(0x01, iface).as<co_usb::ep_direction::in>() == std::nullopt);
    REQUIRE(co_usb::ep_any(0x01, iface).as<co_usb::ep_direction::out>() != std::nullopt);
    REQUIRE(co_usb::ep_any(0x80, iface).as<co_usb::ep_direction::out>() == std::nullopt);
}
