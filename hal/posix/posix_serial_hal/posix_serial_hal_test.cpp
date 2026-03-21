#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <fcntl.h>
#include <thread>
#include <unistd.h>

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

TEST_CASE("send writes data to serial port") {
        int master, slave;
        REQUIRE(open_pty_pair(master, slave) == 0);

        serial::SerialHal hal(ptsname(master));

        const std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        const auto result = hal.send(data);
        REQUIRE(!result.failed());

        std::vector<uint8_t> buf(data.size());
        const auto n = read(master, buf.data(), buf.size());
        REQUIRE(n == (int)data.size());
        REQUIRE(buf == data);

        close(master);
        close(slave);
}

TEST_CASE("loop calls receive callback with data") {
        int master;
        master = posix_openpt(O_RDWR | O_NOCTTY);
        REQUIRE(master >= 0);
        grantpt(master);
        unlockpt(master);

        const char *slave_path = ptsname(master);
        serial::SerialHal hal(slave_path);

        std::vector<uint8_t> received;
        hal.on_receive([&](std::span<const uint8_t> data) {
                received.assign(data.begin(), data.end());
        });

        const std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
        write(master, data.data(), data.size());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        const auto result = hal.loop();
        REQUIRE(!result.failed());
        REQUIRE(received == data);

        close(master);
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
