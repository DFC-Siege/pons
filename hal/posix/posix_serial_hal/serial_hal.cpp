#include "serial_hal.hpp"
#include "result.hpp"
#include <fcntl.h>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>
#include <vector>

namespace serial {

SerialHal::SerialHal(const char *device, int baud_rate,
                     uint16_t max_packet_size, uint32_t max_buffer_size)
    : max_packet_size(max_packet_size), max_buffer_size(max_buffer_size) {
        fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) {
                throw std::runtime_error(
                    std::string("failed to open serial device: ") + device);
        }
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

static result::Try write_all(int fd, const uint8_t *buf, size_t len) {
        while (len > 0) {
                const auto written = write(fd, buf, len);
                if (written < 0)
                        return result::err("failed to write to serial port");
                buf += written;
                len -= static_cast<size_t>(written);
        }
        return result::ok();
}

result::Try SerialHal::send(Data &&data) {
        const uint16_t length = static_cast<uint16_t>(data.size());
        const uint8_t prefix[LENGTH_PREFIX_SIZE] = {
            static_cast<uint8_t>(length & 0xFF),
            static_cast<uint8_t>((length >> 8) & 0xFF)};
        auto r = write_all(fd, prefix, LENGTH_PREFIX_SIZE);
        if (r.failed())
                return r;
        return write_all(fd, data.data(), data.size());
}

void SerialHal::on_receive(ReceiveCallback cb) {
        receive_callback = std::move(cb);
}

result::Try SerialHal::loop() {
        uint8_t tmp[BUF_SIZE];
        const auto length = read(fd, tmp, BUF_SIZE);
        if (length < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                        return result::ok();
                return result::err("failed to read from serial port");
        }
        if (length == 0)
                return result::ok();

        buffer.insert(buffer.end(), tmp, tmp + length);

        if (buffer.size() > max_buffer_size) {
                buffer.clear();
                return result::ok();
        }

        while (buffer.size() >= LENGTH_PREFIX_SIZE) {
                const uint16_t packet_length =
                    static_cast<uint16_t>(buffer[0]) |
                    (static_cast<uint16_t>(buffer[1]) << 8);

                if (packet_length > max_packet_size || packet_length == 0) {
                        buffer.erase(buffer.begin());
                        continue;
                }

                if (buffer.size() < LENGTH_PREFIX_SIZE + packet_length)
                        break;

                Data packet(buffer.begin() + LENGTH_PREFIX_SIZE,
                            buffer.begin() + LENGTH_PREFIX_SIZE +
                                packet_length);
                buffer.erase(buffer.begin(), buffer.begin() +
                                                 LENGTH_PREFIX_SIZE +
                                                 packet_length);

                if (receive_callback)
                        receive_callback(std::move(packet));
        }

        return result::ok();
}

} // namespace serial
