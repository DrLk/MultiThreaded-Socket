#pragma once

#include <cstddef>
#include <cstring>
#include <span>
#include <stop_token>
#include <thread>

#include "Concepts.hpp"
#include "IConnection.hpp"
#include "IPacket.hpp"
#include "LockedList.hpp"

namespace FastTransport::Protocol {

class ConnectionWriter final {
public:
    ConnectionWriter(std::stop_token stop, const IConnection::Ptr& connection);
    ~ConnectionWriter();

    template <trivial T>
    ConnectionWriter& operator<<(const T& trivial); // NOLINT(fuchsia-overloaded-operator)
    ConnectionWriter& operator<<(IPacket::List&& packets); // NOLINT(fuchsia-overloaded-operator)
    ConnectionWriter& flush();

    ConnectionWriter& write(const void* data, std::size_t size);
    IPacket::List GetFreePackets();

private:
    IPacket& GetPacket();
    IPacket::Ptr& GetNextPacket(std::stop_token stop);
    IPacket::List GetPacketToSend(std::stop_token stop);

    static void SendThread(std::stop_token stop, ConnectionWriter& writer, IConnection& connection);

    IConnection::Ptr _connection;
    Containers::LockedList<IPacket::Ptr> _freePackets;
    Containers::LockedList<IPacket::Ptr> _packetsToSend;
    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::ptrdiff_t _offset { 0 };
    std::stop_token _stop;
    bool _error = false;
    std::jthread _sendThread;
};

class PacketReader {
public:
    explicit PacketReader(IPacket::List&& packet)
        : _packets(std::move(packet))
        , _packet(_packets.begin())
    {
    }

    template <trivial T>
    PacketReader& operator>>(T& trivial)
    {
        auto readSize = std::min(sizeof(trivial), GetPacket().GetPayload().size() - _offset);
        std::memcpy(GetPacket().GetPayload().data() + _offset, &trivial, readSize);
        _offset += readSize;

        std::ptrdiff_t size = sizeof(trivial) - readSize;
        if (size) {
            _packet++;
            std::memcpy(((std::byte*)&trivial) + readSize, GetPacket().GetPayload().data(), size);
            _offset = size;
        }
        return *this;
    }

private:
    IPacket& GetPacket()
    {
        return **_packet;
    }

    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::ptrdiff_t _offset { 0 };
};

template <trivial T>
ConnectionWriter& ConnectionWriter::operator<<(const T& trivial) // NOLINT(fuchsia-overloaded-operator)
{
    return write(&trivial, sizeof(trivial));
}

} // namespace FastTransport::Protocol
