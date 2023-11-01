#pragma once

#include <cstdint>

namespace FastTransport::Protocol {

enum class PacketType : int16_t {
    NONE = 0,
    SYN = 1,
    ACK = 2,
    SYN_ACK = 3,
    DATA = 4,
    SACK = 8,
    FIN = 16,
    CLOSE = 32
};

using ConnectionID = std::uint16_t;
using SeqNumberType = std::uint32_t;
using PayloadType = unsigned char;
using PayloadSizeType = std::uint16_t;

static constexpr SeqNumberType MaxAcksSize = 300;
static constexpr SeqNumberType MaxPayloadSize = 1300;
} // namespace FastTransport::Protocol
