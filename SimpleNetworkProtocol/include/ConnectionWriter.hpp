#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <stop_token>
#include <type_traits>

#include "IConnection.hpp"
#include "IPacket.hpp"
#include "LockedList.hpp"

namespace FastTransport::Protocol {

template <class T>
concept trivial = std::is_trivial_v<T>;

class ConnectionWriter final {
public:
    ConnectionWriter(std::stop_token stop, const IConnection::Ptr& connection, IPacket::List&& packet);
    ~ConnectionWriter();

    template <trivial T>
    ConnectionWriter& operator<<(const T& trivial)
    {
        if (_stop.stop_requested()) {
            return *this;
        }

        auto writeSize = std::min(sizeof(trivial), GetPacket().GetPayload().size() - _offset);
        std::memcpy(GetPacket().GetPayload().data() + _offset, &trivial, writeSize);
        _offset += writeSize;

        auto b = sizeof(trivial) - writeSize;
        if (b != 0) {
            IPacket::Ptr& nextPacket = GetNextPacket(_stop);
            if (!nextPacket) {
                _error = true;
                return *this;
            }
            std::memcpy(nextPacket->GetPayload().data(), ((std::byte*)&trivial) + writeSize, b);
            _offset = b;
        }

        return *this;
    }

    ConnectionWriter& operator<<(IPacket::List&& packets)
    {
        if (_stop.stop_requested()) {
            return *this;
        }

        Flush();
        _packetsToSend.LockedSplice(std::move(packets));
        _packetsToSend.NotifyAll();
        return *this;
    }

    void Flush()
    {
        if (_stop.stop_requested()) {
            return;
        }

        IPacket::List packets;
        packets.splice(std::move(_packets), _packets.begin(), _packet);
        _packetsToSend.LockedSplice(std::move(packets));

        auto packet = _packet;
        if (_offset != 0) {
            _packetsToSend.LockedPushBack(std::move(*packet));
        }

        _packetsToSend.NotifyAll();
        _packetsToSend.WaitEmpty(_stop);
        GetNextPacket(_stop);
        packets.erase(packet);
    }

    std::size_t GetFreeSize()
    {
        if (_stop.stop_requested()) {
            return 0;
        }

        return GetPacket().GetPayload().size() - _offset;
    }

private:
    IPacket& GetPacket()
    {
        return **_packet;
    }

    IPacket::Ptr& GetNextPacket(std::stop_token stop)
    {
        auto previouseIterator = _packet;
        _packet++;
        if (_packet == _packets.end()) {
            if (!_freePackets.Wait(stop)) {
                static IPacket::Ptr null;
                return null;
            }
            IPacket::List freePackets;
            _freePackets.LockedSwap(freePackets);

            _packets.splice(std::move(freePackets));
            previouseIterator++;
            _packet = previouseIterator;
        }

        assert(_packet != _packets.end());

        return *_packet;
    }

    IPacket::List GetPacketToSend(std::stop_token stop)
    {
        IPacket::List result;
        _packetsToSend.Wait(stop);
        _packetsToSend.LockedSwap(result);
        _packetsToSend.NotifyAll();
        return result;
    }

    static void SendThread(std::stop_token stop, ConnectionWriter& writer, IConnection& connection)
    {
        while (!stop.stop_requested()) {
            IPacket::List packets = writer.GetPacketToSend(stop);
            IPacket::List freePackets = connection.Send(stop, std::move(packets));

            writer._freePackets.LockedSplice(std::move(freePackets));
            writer._freePackets.NotifyAll();
        }
    }

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

        auto b = sizeof(trivial) - readSize;
        if (b) {
            _packet++;
            std::memcpy(((std::byte*)&trivial) + readSize, GetPacket().GetPayload().data(), b);
            _offset = b;
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

} // namespace FastTransport::Protocol
