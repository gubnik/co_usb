// clang-format off
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <co_usb/co_usb.hpp>
#include "co_usb/transfer/detail/device_handle_source.hpp"
#include "test_mock.hpp"
// clang-format on

struct devh_provider
{
    auto devh () const noexcept -> libusb_device_handle *
    {
        return mock<libusb_device_handle *>();
    }
};
static_assert(co_usb::detail::DeviceHandleSource<devh_provider>, "Not a Device Source");

static devh_provider iface{};

TEST_CASE("ep-safe", "[endpoint]")
{
    REQUIRE(co_usb::endpoint<co_usb::endpoint_direction::in>::make_safe(0x81, iface).addr() ==
            0x81);
    REQUIRE(co_usb::endpoint<co_usb::endpoint_direction::in>::make_safe(0x01, iface).addr() ==
            0x81);
    REQUIRE(co_usb::endpoint<co_usb::endpoint_direction::in>::make_safe(0x00, iface).addr() ==
            0x80);
    REQUIRE(co_usb::endpoint<co_usb::endpoint_direction::in>::make_safe(0x80, iface).addr() ==
            0x80);

    REQUIRE(co_usb::endpoint<co_usb::endpoint_direction::out>::make_safe(0x81, iface).addr() ==
            0x01);
    REQUIRE(co_usb::endpoint<co_usb::endpoint_direction::out>::make_safe(0x01, iface).addr() ==
            0x01);
    REQUIRE(co_usb::endpoint<co_usb::endpoint_direction::out>::make_safe(0x00, iface).addr() ==
            0x00);
    REQUIRE(co_usb::endpoint<co_usb::endpoint_direction::out>::make_safe(0x80, iface).addr() ==
            0x00);
}

TEST_CASE("ep-throwing", "[endpoint]")
{
    libusb_device_handle *ndevh = nullptr;
    REQUIRE_NOTHROW(co_usb::endpoint<co_usb::endpoint_direction::in>::make_throwing(0x80, ndevh));
    REQUIRE_NOTHROW(co_usb::endpoint<co_usb::endpoint_direction::in>::make_throwing(0x81, ndevh));
    REQUIRE_THROWS_AS(co_usb::endpoint<co_usb::endpoint_direction::in>::make_throwing(0x00, ndevh),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(co_usb::endpoint<co_usb::endpoint_direction::in>::make_throwing(0x01, ndevh),
                      std::invalid_argument);

    REQUIRE_NOTHROW(co_usb::endpoint<co_usb::endpoint_direction::out>::make_throwing(0x00, ndevh));
    REQUIRE_NOTHROW(co_usb::endpoint<co_usb::endpoint_direction::out>::make_throwing(0x01, ndevh));
    REQUIRE_THROWS_AS(co_usb::endpoint<co_usb::endpoint_direction::out>::make_throwing(0x80, ndevh),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(co_usb::endpoint<co_usb::endpoint_direction::out>::make_throwing(0x81, ndevh),
                      std::invalid_argument);
}

TEST_CASE("ep-conversion", "[endpoint]")
{
    REQUIRE(co_usb::endpoint_any(0x81, iface).as<co_usb::endpoint_direction::in>() != std::nullopt);
    REQUIRE(co_usb::endpoint_any(0x01, iface).as<co_usb::endpoint_direction::in>() == std::nullopt);
    REQUIRE(co_usb::endpoint_any(0x01, iface).as<co_usb::endpoint_direction::out>() !=
            std::nullopt);
    REQUIRE(co_usb::endpoint_any(0x80, iface).as<co_usb::endpoint_direction::out>() ==
            std::nullopt);
}
