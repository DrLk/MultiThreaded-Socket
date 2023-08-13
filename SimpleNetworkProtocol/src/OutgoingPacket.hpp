#pragma once

#include <chrono>

#include "IPacket.hpp"
#include "MultiList.hpp" // IWYU pragma: export

namespace FastTransport::Protocol {

class OutgoingPacket {
    using clock = std::chrono::steady_clock;

public:
    using List = FastTransport::Containers::MultiList<OutgoingPacket>;

    OutgoingPacket() = default;
    OutgoingPacket(const OutgoingPacket&) = delete;
    OutgoingPacket(OutgoingPacket&&) = default;
    OutgoingPacket(IPacket::Ptr&& packet, bool needAck);
    OutgoingPacket& operator=(const OutgoingPacket&) = delete;
    OutgoingPacket& operator=(OutgoingPacket&&) = default;
    ~OutgoingPacket() = default;

    const IPacket::Ptr& GetPacket() const;
    IPacket::Ptr& GetPacket();

    clock::time_point _sendTime; // NOLINT
    bool _needAck {}; // NOLINT
private:
    IPacket::Ptr _packet;
};
} // namespace FastTransport::Protocol
