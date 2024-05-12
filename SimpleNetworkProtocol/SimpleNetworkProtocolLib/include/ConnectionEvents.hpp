#pragma once

namespace FastTransport::Protocol {

class ConnectionEvents {
public:
    ConnectionEvents() = default;
    ConnectionEvents(const ConnectionEvents&) = default;
    ConnectionEvents(ConnectionEvents&&) = default;
    ConnectionEvents& operator=(const ConnectionEvents&) = default;
    ConnectionEvents& operator=(ConnectionEvents&&) = default;
    virtual ~ConnectionEvents() = default;
    virtual void OnSendPacket() = 0;
};

} // namespace FastTransport::Protocol
