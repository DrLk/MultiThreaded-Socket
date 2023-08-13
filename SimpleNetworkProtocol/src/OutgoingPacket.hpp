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

    [[nodiscard]] const IPacket::Ptr& GetPacket() const;
    IPacket::Ptr& GetPacket();
    [[nodiscard]] bool NeedAck() const;
    [[nodiscard]] clock::time_point GetSendTime() const;
    void SetSendTime(clock::time_point sendTime);

private:
    IPacket::Ptr _packet;
    clock::time_point _sendTime;
    bool _needAck {};
};
} // namespace FastTransport::Protocol
