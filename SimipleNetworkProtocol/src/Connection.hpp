#pragma once

#include <chrono>
#include <list>
#include <memory>
#include <vector>

#include "ConnectionKey.hpp"
#include "IInFilghtQueue.hpp"
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "LockedList.hpp"
#include "Packet.hpp"
#include "SendQueue.hpp"

namespace FastTransport::Protocol {

using namespace std::chrono_literals;
class IConnectionState;

class IConnection {
public:
    virtual void Send(IPacket::Ptr&& data) = 0;
    virtual IPacket::List Send(IPacket::List&& data) = 0;
    virtual IPacket::List Recv(IPacket::List&& freePackets) = 0;
};

class Connection : public IConnection {
public:
    Connection(IConnectionState* state, const ConnectionAddr& addr, ConnectionID myID)
        : _state(state)
        , _key(addr, myID)
        , _destinationID(0)
        , _lastReceivedPacket(DefaultTimeOut)
    {
        for (int i = 0; i < 1000; i++) {
            _freeSendPackets.push_back(std::move(std::make_unique<Packet>(1500)));
            _freeRecvPackets.push_back(std::move(std::make_unique<Packet>(1500)));
        }
    }

    void Send(IPacket::Ptr&& data) override;
    IPacket::List Send(IPacket::List&& data) override;

    IPacket::List Recv(IPacket::List&& freePackets) override;

    IPacket::List OnRecvPackets(IPacket::Ptr&& packet);

    const ConnectionKey& GetConnectionKey() const;
    OutgoingPacket::List GetPacketsToSend();

    void ProcessSentPackets(OutgoingPacket::List&& packets);
    void ProcessRecvPackets();

    void Close();

    void Run();

    void SendPacket(IPacket::Ptr&& packet, bool needAck);

    IInflightQueue& GetInFlightQueue()
    {
        return _inFlightQueue;
    }

    RecvQueue& GetRecvQueue()
    {
        return _recvQueue;
    }

    SendQueue& GetSendQueue()
    {
        return _sendQueue;
    }

    IConnectionState* _state; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    IPacket::List _sendUserData; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
    std::mutex _sendUserDataMutex; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    ConnectionKey _key; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
    ConnectionID _destinationID; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    LockedList<IPacket::Ptr> _freeSendPackets; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
    LockedList<IPacket::Ptr> _freeRecvPackets; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
private:
    LockedList<std::vector<char>> _recvUserData;

    std::chrono::microseconds _lastReceivedPacket;

    static constexpr std::chrono::microseconds DefaultTimeOut = 100ms;

    IInflightQueue _inFlightQueue;
    RecvQueue _recvQueue;
    SendQueue _sendQueue;
};
} // namespace FastTransport::Protocol
