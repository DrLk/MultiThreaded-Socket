#pragma once

#include <list>
#include <memory>

#include "HeaderBuffer.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {
class Connection;
class Packet;

enum ConnectionState {
    WAIT_SYNACK,
    CONNECTED,
};

class IConnectionState {
public:
    IConnectionState() = default;
    IConnectionState(const IConnectionState&) = default;
    IConnectionState(IConnectionState&&) = default;
    IConnectionState& operator=(const IConnectionState&) = default;
    IConnectionState& operator=(IConnectionState&&) = default;
    virtual ~IConnectionState() = default;

    virtual IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) = 0;
    virtual IConnectionState* SendPackets(Connection& connection) = 0;
    virtual IConnectionState* OnTimeOut(Connection& connection) = 0;
    virtual void ProcessInflightPackets(Connection& connection) = 0;
};

class BasicConnectionState : public IConnectionState {
public:
    IPacket::List OnRecvPackets(IPacket::Ptr&& /*packet*/, Connection& /*connection*/) override
    {
        throw std::runtime_error("Not implemented");
    }
    IConnectionState* SendPackets(Connection& /*connection*/) override { return this; }
    IConnectionState* OnTimeOut(Connection& /*connection*/) override { return this; }
    void ProcessInflightPackets(Connection& connection) override;
};

class ListenState {
public:
    static std::pair<Connection*, IPacket::List> Listen(IPacket::Ptr&& packet, ConnectionID myID);
};

class SendingSynState final : public BasicConnectionState {
public:
    IConnectionState* SendPackets(Connection& connection) override;
};

class WaitingSynState final : public BasicConnectionState {
public:
    IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
};

class WaitingSynAckState final : public BasicConnectionState {
public:
    IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
    IConnectionState* OnTimeOut(Connection& connection) override;
};

class SendingSynAckState final : public BasicConnectionState {
    IConnectionState* SendPackets(Connection& connection) override;
};

class DataState final : public BasicConnectionState {
public:
    IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
    IConnectionState* SendPackets(Connection& connection) override;
    IConnectionState* OnTimeOut(Connection& connection) override;
};

class ClosingState final : public BasicConnectionState {
public:
    IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
};

class ClosedState final : public BasicConnectionState {
public:
    IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
};
} // namespace FastTransport::Protocol
