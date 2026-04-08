#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "serial_hal.hpp"

static int open_pty_pair(int &master, int &slave) {
        master = posix_openpt(O_RDWR | O_NOCTTY);
        if (master < 0)
                return -1;
        grantpt(master);
        unlockpt(master);
        slave = open(ptsname(master), O_RDWR | O_NOCTTY);
        return slave < 0 ? -1 : 0;
}

TEST_CASE("send writes data with length prefix to serial port") {
        int master, slave;
        REQUIRE(open_pty_pair(master, slave) == 0);

        serial::SerialHal hal(ptsname(master));

        std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        const auto result = hal.send(std::vector<uint8_t>(data));
        REQUIRE(!result.failed());

        std::vector<uint8_t> expected = {0x03, 0x00, 0x01, 0x02, 0x03};
        std::vector<uint8_t> buf(expected.size());

        const auto n = read(master, buf.data(), buf.size());
        REQUIRE(n == (int)expected.size());
        REQUIRE(buf == expected);

        close(master);
        close(slave);
}

TEST_CASE("loop calls receive callback with length-prefixed data") {
        int master, slave;
        REQUIRE(open_pty_pair(master, slave) == 0);

        serial::SerialHal hal(ptsname(master));

        std::vector<uint8_t> received;
        hal.on_receive(
            [&](std::vector<uint8_t> data) { received = std::move(data); });

        std::vector<uint8_t> raw_to_send = {0x04, 0x00, 0xDE, 0xAD, 0xBE, 0xEF};
        write(master, raw_to_send.data(), raw_to_send.size());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        const auto result = hal.loop();
        REQUIRE(!result.failed());

        std::vector<uint8_t> expected_payload = {0xDE, 0xAD, 0xBE, 0xEF};
        REQUIRE(received == expected_payload);

        close(master);
        close(slave);
}

TEST_CASE("loop returns ok when no data available") {
        int master, slave;
        REQUIRE(open_pty_pair(master, slave) == 0);

        serial::SerialHal hal(ptsname(master));
        const auto result = hal.loop();
        REQUIRE(!result.failed());

        close(master);
        close(slave);
}

TEST_CASE("loop recovers from desync garbage byte") {
        int master, slave;
        REQUIRE(open_pty_pair(master, slave) == 0);

        serial::SerialHal hal(ptsname(master));

        std::vector<uint8_t> received;
        hal.on_receive(
            [&](std::vector<uint8_t> data) { received = std::move(data); });

        // Write a garbage byte followed by a valid packet
        std::vector<uint8_t> raw = {
            0xFF,                         // garbage byte
            0x02, 0x00, 0xAA, 0xBB        // valid 2-byte packet
        };
        write(master, raw.data(), raw.size());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // May need multiple loop calls to resync
        for (int i = 0; i < 5 && received.empty(); ++i) {
                hal.loop();
        }

        REQUIRE(received == std::vector<uint8_t>{0xAA, 0xBB});

        close(master);
        close(slave);
}

TEST_CASE("loop handles multiple packets in one read") {
        int master, slave;
        REQUIRE(open_pty_pair(master, slave) == 0);

        serial::SerialHal hal(ptsname(master));

        std::vector<std::vector<uint8_t>> received;
        hal.on_receive([&](std::vector<uint8_t> data) {
                received.push_back(std::move(data));
        });

        // Two back-to-back packets
        std::vector<uint8_t> raw = {
            0x02, 0x00, 0x01, 0x02,       // packet 1: [0x01, 0x02]
            0x01, 0x00, 0x03              // packet 2: [0x03]
        };
        write(master, raw.data(), raw.size());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        hal.loop();

        REQUIRE(received.size() == 2);
        REQUIRE(received[0] == std::vector<uint8_t>{0x01, 0x02});
        REQUIRE(received[1] == std::vector<uint8_t>{0x03});

        close(master);
        close(slave);
}

TEST_CASE("loop clears buffer when max_buffer_size exceeded") {
        int master, slave;
        REQUIRE(open_pty_pair(master, slave) == 0);

        // Small max_buffer_size to trigger overflow easily
        serial::SerialHal hal(ptsname(master), B115200, 512, 16);

        std::vector<uint8_t> received;
        hal.on_receive(
            [&](std::vector<uint8_t> data) { received = std::move(data); });

        // Write more than 16 bytes of garbage to overflow the buffer
        std::vector<uint8_t> garbage(20, 0xFF);
        write(master, garbage.data(), garbage.size());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        hal.loop();

        // Buffer was cleared, no packets dispatched
        REQUIRE(received.empty());

        // Now send a valid packet — should work after the clear
        std::vector<uint8_t> valid = {0x02, 0x00, 0xAA, 0xBB};
        write(master, valid.data(), valid.size());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        hal.loop();

        REQUIRE(received == std::vector<uint8_t>{0xAA, 0xBB});

        close(master);
        close(slave);
}

TEST_CASE("loop does not clear buffer within max_buffer_size") {
        int master, slave;
        REQUIRE(open_pty_pair(master, slave) == 0);

        serial::SerialHal hal(ptsname(master), B115200, 512, 2048);

        std::vector<uint8_t> received;
        hal.on_receive(
            [&](std::vector<uint8_t> data) { received = std::move(data); });

        // Send a valid packet well within the buffer limit
        std::vector<uint8_t> raw = {0x03, 0x00, 0x01, 0x02, 0x03};
        write(master, raw.data(), raw.size());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        hal.loop();

        REQUIRE(received == std::vector<uint8_t>{0x01, 0x02, 0x03});

        close(master);
        close(slave);
}
