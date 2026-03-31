#include <cstdint>
#include <span>

#include "chunked_transporter.hpp"
#include "packet.hpp"
#include "promise.hpp"
#include "result.hpp"
#include "transporter.hpp"

namespace transport {
template <Transporter T>
ChunkedTransporter<T>::ChunkedTransporter(T &transporter, uint16_t mtu,
                                          uint8_t max_attempts)
    : transporter(transporter), mtu(mtu), max_attempts(max_attempts) {
}

template <Transporter T>
result::Result<bool>
ChunkedTransporter<T>::send(std::span<const uint8_t> data) {
        return result::err("not implemented");
}

result::Result<std::vector<uint8_t>> receive() {
        return result::err("not implemented");
}
} // namespace transport
