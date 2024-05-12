#pragma once

namespace FastTransport::Protocol {

class UDPQueueEvents {
public:
    UDPQueueEvents() = default;
    UDPQueueEvents(const UDPQueueEvents&) = default;
    UDPQueueEvents(UDPQueueEvents&&) = delete;
    UDPQueueEvents& operator=(const UDPQueueEvents&) = default;
    UDPQueueEvents& operator=(UDPQueueEvents&&) = delete;
    virtual ~UDPQueueEvents() = default;

    virtual void OnOutgoingPackets() = 0;
};

} // namespace FastTransport::Protocol
