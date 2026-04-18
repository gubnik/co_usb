#include <array>
#include <boost/capy.hpp>
#include <chrono>
#include <co_usb.hpp>
#include <print>

constexpr uint8_t total     = 8;
constexpr uint16_t dev_vid  = 0x9f9f;
constexpr uint16_t dev_pid  = 0x9f9f;
constexpr uint8_t dev_ep    = 0x81;
constexpr uint8_t dev_iface = 0;

boost::capy::task<void> process_transfer (libusb_device_handle *devh)
{
    auto st = co_await boost::capy::this_coro::stop_token;
    std::array<uint8_t, 16 * 16> data;
    // create a transfer (100% syntactic suger over transfer_awaitable)
    co_usb::bulk_transfer<co_usb::ep_direction::in> tfer{{0x81, devh},
                                                         std::chrono::milliseconds{0'050}};
    while (!st.stop_requested())
    {
        auto [ec, n] = co_await tfer.read_some({data.data(), data.size()});
        if (ec)
        {
            std::println("Got error: {}", ec.message());
            continue;
        }
        std::println("Got data: {}", std::string_view{(char *)data.data(), n});
    }
    std::println("Gracefully exited");
}

int main (int argc, char **argv)
{
    co_usb::context ctx;
    co_usb::unique_dev_handle devh = co_usb::open_vid_pid(ctx.get(), dev_vid, dev_pid);
    if (!devh)
    {
        std::println(stderr, "Cannot open device!");
        return 1;
    }
    // claim interface and release on scope end (RAII)
    co_usb::interface_holder _{devh.get(), 0};

    // spawn a default handler thread
    // co_usb gives you an option to NOT do this and provide your own!
    ctx.spawn_handler_thread();
    boost::capy::thread_pool tp{total};
    for (uint8_t i = 0; i < total; i++)
    {
        boost::capy::run_async(tp.get_executor(), ctx.get_token())(process_transfer(devh.get()));
    }
    tp.join();
}
