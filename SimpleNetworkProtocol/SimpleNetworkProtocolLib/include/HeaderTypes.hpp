#pragma once

#include <cstddef>
#include <cstdint>

namespace FastTransport::Protocol {

enum class PacketType : int16_t {
    None = 0,
    Syn = 1,
    Ack = 2,
    SynAck = 3,
    Data = 4,
    Sack = 8,
    Fin = 16,
    Close = 32
};

using MagicNumber = int;
using ConnectionID = std::uint16_t;
using SeqNumberType = std::uint32_t;
using PayloadType = std::byte;
using PayloadSizeType = std::uint16_t;

static constexpr SeqNumberType MaxAcksSize = 300;
static constexpr std::size_t MaxPayloadSize = 1300;

static constexpr size_t HeaderSize = sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType) + sizeof(SeqNumberType) + sizeof(PayloadSizeType);
} // namespace FastTransport::Protocol
