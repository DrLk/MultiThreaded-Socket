#pragma once

#include <cstddef>
#include <stop_token>

#include "IPacket.hpp"

namespace FastTransport::Protocol {

class ConnectionContext;
class IStatistics;

class IConnection {
public:
    using Ptr = std::shared_ptr<IConnection>;

    IConnection() = default;
    IConnection(const IConnection&) = default;
    IConnection(IConnection&&) = default;
    IConnection& operator=(const IConnection&) = default;
    IConnection& operator=(IConnection&&) = default;
    virtual ~IConnection() = default;

    [[nodiscard]] virtual bool IsConnected() const = 0;
    [[nodiscard]] virtual const IStatistics& GetStatistics() const = 0;
    [[nodiscard]] virtual ConnectionContext& GetContext() = 0;

    [[nodiscard]] virtual IPacket::List GetFreeSendPackets(std::stop_token stop) = 0;
    virtual void Send(IPacket::List&& data) = 0;
    [[nodiscard]] virtual IPacket::List Send2(std::stop_token stop, IPacket::List&& data) = 0;
    [[nodiscard]] virtual IPacket::List Recv(std::stop_token stop, IPacket::List&& freePackets) = 0;
    [[nodiscard]] virtual IPacket::List Recv(std::size_t size, std::stop_token stop, IPacket::List&& freePackets) = 0;

    virtual void AddFreeRecvPackets(IPacket::List&& freePackets) = 0;
    virtual void AddFreeSendPackets(IPacket::List&& freePackets) = 0;

    virtual void Close() = 0;
    [[nodiscard]] virtual bool IsClosed() const = 0;
};
} // namespace FastTransport::Protocol
