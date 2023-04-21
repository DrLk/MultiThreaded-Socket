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

using ConnectionID = uint16_t;
using SeqNumberType = unsigned int;

static constexpr SeqNumberType MaxAcks = 300;
} // namespace FastTransport::Protocol
