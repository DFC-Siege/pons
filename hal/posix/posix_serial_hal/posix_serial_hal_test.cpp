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
