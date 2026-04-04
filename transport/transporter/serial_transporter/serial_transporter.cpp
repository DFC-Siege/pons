#include <cstdint>

#include "result.hpp"
#include "serial_transporter.hpp"
#include "transporter/base_transporter.hpp"

namespace transport {

SerialTransporter::SerialTransporter(serial::ISerialHal &serial_hal)
    : BaseTransporter(), serial_hal(serial_hal) {
        serial_hal.on_receive(
            [this](Data data) { this->handle_receive(std::move(data)); });
}

result::Result<bool> SerialTransporter::send(Data &&data) {
        return this->serial_hal.send(std::move(data));
}

MTU SerialTransporter::get_mtu() const {
        // TODO: Get from correct place
        return 512;
}
} // namespace transport
