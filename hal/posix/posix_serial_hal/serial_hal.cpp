#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include "result.hpp"
#include "serial_hal.hpp"

namespace serial {
SerialHal::SerialHal(const char *device, int baud_rate) {
        fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);

        termios tty{};
        tcgetattr(fd, &tty);
        cfsetispeed(&tty, baud_rate);
        cfsetospeed(&tty, baud_rate);
        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
        tty.c_cflag |= CLOCAL | CREAD;
        tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS);
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_lflag = 0;
        tty.c_oflag = 0;
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 0;
        tcsetattr(fd, TCSANOW, &tty);
}

SerialHal::~SerialHal() {
        if (fd >= 0)
                close(fd);
}

result::Result<bool> SerialHal::send(Data &&data) {
        const auto written = write(fd, data.data(), data.size());
        if (written < 0)
                return result::err("failed to write to serial port");
        return result::ok();
}

void SerialHal::on_receive(ReceiveCallback cb) {
        receive_callback = std::move(cb);
}

result::Result<bool> SerialHal::loop() {
        std::vector<uint8_t> data(BUF_SIZE);
        const auto length = read(fd, data.data(), BUF_SIZE);
        if (length < 0)
                return result::err("failed to read from serial port");
        if (length == 0)
                return result::ok();
        data.resize(length);
        if (receive_callback)
                receive_callback(std::move(data));
        return result::ok();
}
} // namespace serial
