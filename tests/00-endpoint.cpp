// clang-format off
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <co_usb.hpp>
#include "co_usb/transfer/endpoint.hpp"
#include "test_mock.hpp"
// clang-format on

static co_usb::interface const &iface = mock<co_usb::interface>(); // dummy

TEST_CASE("ep-safe", "[endpoint]")
{
    REQUIRE(co_usb::endpoint<co_usb::ep_direction::in>::make_safe(0x81, iface).addr() == 0x81);
    REQUIRE(co_usb::endpoint<co_usb::ep_direction::in>::make_safe(0x01, iface).addr() == 0x81);
    REQUIRE(co_usb::endpoint<co_usb::ep_direction::in>::make_safe(0x00, iface).addr() == 0x80);
    REQUIRE(co_usb::endpoint<co_usb::ep_direction::in>::make_safe(0x80, iface).addr() == 0x80);

    REQUIRE(co_usb::endpoint<co_usb::ep_direction::out>::make_safe(0x81, iface).addr() == 0x01);
    REQUIRE(co_usb::endpoint<co_usb::ep_direction::out>::make_safe(0x01, iface).addr() == 0x01);
    REQUIRE(co_usb::endpoint<co_usb::ep_direction::out>::make_safe(0x00, iface).addr() == 0x00);
    REQUIRE(co_usb::endpoint<co_usb::ep_direction::out>::make_safe(0x80, iface).addr() == 0x00);
}

TEST_CASE("ep-throwing", "[endpoint]")
{
    REQUIRE_NOTHROW(co_usb::endpoint<co_usb::ep_direction::in>::make_throwing(0x80, nullptr));
    REQUIRE_NOTHROW(co_usb::endpoint<co_usb::ep_direction::in>::make_throwing(0x81, nullptr));
    REQUIRE_THROWS_AS(co_usb::endpoint<co_usb::ep_direction::in>::make_throwing(0x00, nullptr),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(co_usb::endpoint<co_usb::ep_direction::in>::make_throwing(0x01, nullptr),
                      std::invalid_argument);

    REQUIRE_NOTHROW(co_usb::endpoint<co_usb::ep_direction::out>::make_throwing(0x00, nullptr));
    REQUIRE_NOTHROW(co_usb::endpoint<co_usb::ep_direction::out>::make_throwing(0x01, nullptr));
    REQUIRE_THROWS_AS(co_usb::endpoint<co_usb::ep_direction::out>::make_throwing(0x80, nullptr),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(co_usb::endpoint<co_usb::ep_direction::out>::make_throwing(0x81, nullptr),
                      std::invalid_argument);
}

TEST_CASE("ep-conversion", "[endpoint]")
{
    REQUIRE(co_usb::ep_any(0x81, iface).as<co_usb::ep_direction::in>() != std::nullopt);
    REQUIRE(co_usb::ep_any(0x01, iface).as<co_usb::ep_direction::in>() == std::nullopt);
    REQUIRE(co_usb::ep_any(0x01, iface).as<co_usb::ep_direction::out>() != std::nullopt);
    REQUIRE(co_usb::ep_any(0x80, iface).as<co_usb::ep_direction::out>() == std::nullopt);
}
