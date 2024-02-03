#pragma once

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

    [[nodiscard]] virtual IPacket::List Send(std::stop_token stop, IPacket::List&& data) = 0;
    [[nodiscard]] virtual IPacket::List Recv(std::stop_token stop, IPacket::List&& freePackets) = 0;

    virtual void Close() = 0;
    [[nodiscard]] virtual bool IsClosed() const = 0;
};
} // namespace FastTransport::Protocol