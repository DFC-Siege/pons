#include <cstdint>

#include "result.hpp"
#include "serial_transporter.hpp"

namespace transport {

SerialTransporter::SerialTransporter(serial::ISerialHal &serial_hal)
    : serial_hal(serial_hal) {
}

result::Result<bool> SerialTransporter::send(std::span<const uint8_t> data) {
        return this->serial_hal.send(data);
}

result::Result<std::vector<uint8_t>> SerialTransporter::receive() {
        return serial_hal.read(TIMEOUT);
}
} // namespace transport
