#pragma once

#include <chrono>

#include "IPacket.hpp"
#include "MultiList.hpp"

namespace FastTransport::Protocol {
class OutgoingPacket {
    using clock = std::chrono::steady_clock;

public:
    using List = FastTransport::Containers::MultiList<OutgoingPacket>;

    OutgoingPacket() = default;
    OutgoingPacket(IPacket::Ptr&& packet, bool needAck)
        : _packet(std::move(packet))
        , _needAck(needAck)
    {
    }

    IPacket::Ptr _packet; // NOLINT
    clock::time_point _sendTime; // NOLINT
    bool _needAck {}; // NOLINT
};
} // namespace FastTransport::Protocol
