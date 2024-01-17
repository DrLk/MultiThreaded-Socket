#pragma once

#include <chrono>
#include <memory>
#include <tuple>

#include "ConnectionState.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {

class Connection;
class IConnectionInternal;

class IConnectionState {
public:
    IConnectionState() = default;
    IConnectionState(const IConnectionState&) = default;
    IConnectionState(IConnectionState&&) = default;
    IConnectionState& operator=(const IConnectionState&) = default;
    IConnectionState& operator=(IConnectionState&&) = default;
    virtual ~IConnectionState() = default;

    virtual std::tuple<ConnectionState, IPacket::List> OnRecvPackets(IPacket::Ptr&& packet, IConnectionInternal& connection) = 0;
    virtual ConnectionState SendPackets(IConnectionInternal& connection) = 0;
    virtual ConnectionState OnTimeOut(IConnectionInternal& connection) = 0;
    [[nodiscard]] virtual std::chrono::milliseconds GetTimeout() const = 0;
    virtual void ProcessInflightPackets(IConnectionInternal& connection) = 0;
};

class BasicConnectionState : public IConnectionState {
public:
    std::tuple<ConnectionState, IPacket::List> OnRecvPackets(IPacket::Ptr&& /*packet*/, IConnectionInternal& /*connection*/) override;
    ConnectionState SendPackets(IConnectionInternal& /*connection*/) override;
    ConnectionState OnTimeOut(IConnectionInternal& /*connection*/) override;
    [[nodiscard]] std::chrono::milliseconds GetTimeout() const override;
    void ProcessInflightPackets(IConnectionInternal& connection) override;
};

class ListenState {
public:
    static std::pair<std::shared_ptr<Connection>, IPacket::List> Listen(IPacket::Ptr&& packet, ConnectionID myID);
};

class SendingSynState final : public BasicConnectionState {
public:
    ConnectionState SendPackets(IConnectionInternal& connection) override;
};

class WaitingSynState final : public BasicConnectionState {
public:
    std::tuple<ConnectionState, IPacket::List> OnRecvPackets(IPacket::Ptr&& packet, IConnectionInternal& connection) override;
};

class WaitingSynAckState final : public BasicConnectionState {
public:
    std::tuple<ConnectionState, IPacket::List> OnRecvPackets(IPacket::Ptr&& packet, IConnectionInternal& connection) override;
    ConnectionState SendPackets(IConnectionInternal& connection) override;
    ConnectionState OnTimeOut(IConnectionInternal& connection) override;
};

class SendingSynAckState final : public BasicConnectionState {
public:
    ConnectionState SendPackets(IConnectionInternal& connection) override;
};

class DataState final : public BasicConnectionState {
public:
    std::tuple<ConnectionState, IPacket::List> OnRecvPackets(IPacket::Ptr&& packet, IConnectionInternal& connection) override;
    ConnectionState SendPackets(IConnectionInternal& connection) override;
    [[nodiscard]] ConnectionState OnTimeOut(IConnectionInternal& connection) override;
    [[nodiscard]] std::chrono::milliseconds GetTimeout() const override;
};

class ClosingState final : public BasicConnectionState {
public:
};

class ClosedState final : public BasicConnectionState {
public:
};
} // namespace FastTransport::Protocol
