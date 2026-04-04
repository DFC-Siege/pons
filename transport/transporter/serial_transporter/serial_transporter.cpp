#include <cstdint>

#include "result.hpp"
#include "serial_transporter.hpp"
#include "transporter/base_transporter.hpp"

namespace transport {

SerialTransporter::SerialTransporter(serial::ISerialHal &serial_hal)
    : BaseTransporter(), serial_hal(serial_hal) {
        serial_hal.on_receive(
            [this](DataView data) { this->handle_receive(data); });
}

result::Result<bool> SerialTransporter::send(DataView data) {
        return this->serial_hal.send(data);
}

MTU SerialTransporter::get_mtu() const {
        // TODO: Get from correct place
        return 512;
}
} // namespace transport
