#pragma once

#include <chrono>
#include <memory>
#include <tuple>

#include "ConnectionState.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {

class Connection;

class IConnectionState {
public:
    IConnectionState() = default;
    IConnectionState(const IConnectionState&) = default;
    IConnectionState(IConnectionState&&) = default;
    IConnectionState& operator=(const IConnectionState&) = default;
    IConnectionState& operator=(IConnectionState&&) = default;
    virtual ~IConnectionState() = default;

    virtual std::tuple<ConnectionState, IPacket::List> OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) = 0;
    virtual ConnectionState SendPackets(Connection& connection) = 0;
    virtual ConnectionState OnTimeOut(Connection& connection) = 0;
    [[nodiscard]] virtual std::chrono::milliseconds GetTimeout() const = 0;
    virtual void ProcessInflightPackets(Connection& connection) = 0;
};

class BasicConnectionState : public IConnectionState {
public:
    std::tuple<ConnectionState, IPacket::List> OnRecvPackets(IPacket::Ptr&& /*packet*/, Connection& /*connection*/) override;
    ConnectionState SendPackets(Connection& /*connection*/) override;
    ConnectionState OnTimeOut(Connection& /*connection*/) override;
    [[nodiscard]] std::chrono::milliseconds GetTimeout() const override;
    void ProcessInflightPackets(Connection& connection) override;
};

class ListenState {
public:
    static std::pair<std::shared_ptr<Connection>, IPacket::List> Listen(IPacket::Ptr&& packet, ConnectionID myID);
};

class SendingSynState final : public BasicConnectionState {
public:
    ConnectionState SendPackets(Connection& connection) override;
};

class WaitingSynState final : public BasicConnectionState {
public:
    std::tuple<ConnectionState, IPacket::List> OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
};

class WaitingSynAckState final : public BasicConnectionState {
public:
    std::tuple<ConnectionState, IPacket::List> OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
    ConnectionState SendPackets(Connection& connection) override;
    ConnectionState OnTimeOut(Connection& connection) override;
};

class SendingSynAckState final : public BasicConnectionState {
    ConnectionState SendPackets(Connection& connection) override;
};

class DataState final : public BasicConnectionState {
public:
    std::tuple<ConnectionState, IPacket::List> OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
    ConnectionState SendPackets(Connection& connection) override;
    [[nodiscard]] ConnectionState OnTimeOut(Connection& connection) override;
    [[nodiscard]] std::chrono::milliseconds GetTimeout() const override;
};

class ClosingState final : public BasicConnectionState {
public:
};

class ClosedState final : public BasicConnectionState {
public:
};
} // namespace FastTransport::Protocol
