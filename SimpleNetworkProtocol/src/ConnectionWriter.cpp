#include "ConnectionWriter.hpp"

#include <functional>
#include <thread>
#include <utility>

namespace FastTransport::Protocol {
ConnectionWriter::ConnectionWriter(std::stop_token stop, const IConnection::Ptr& connection, IPacket::List&& packet)
    : _connection(connection)
    , _packets(std::move(packet))
    , _packet(_packets.begin())
      ,_stop(std::move(stop))
    , _sendThread(SendThread, std::ref(*this), std::ref(*connection))
{
}

ConnectionWriter::~ConnectionWriter()
{
    Flush();
}

ConnectionWriter& ConnectionWriter::operator<<(IPacket::List&& packets) // NOLINT(fuchsia-overloaded-operator)
{
    if (_stop.stop_requested()) {
        return *this;
    }

    Flush();
    _packetsToSend.LockedSplice(std::move(packets));
    _packetsToSend.NotifyAll();
    return *this;
}

void ConnectionWriter::Flush()
{
    if (_stop.stop_requested()) {
        return;
    }

    if (_packets.begin() != _packet) {
        IPacket::List packets;
        packets.splice(std::move(_packets), _packets.begin(), _packet);
        _packetsToSend.LockedSplice(std::move(packets));
    }

    auto packet = _packet;
    if (_offset != 0) {
        _packetsToSend.LockedPushBack(std::move(*packet));
        GetNextPacket(_stop);
        _packets.erase(packet);
    }

    _packetsToSend.NotifyAll();
    _packetsToSend.WaitEmpty(_stop);
}

IPacket::List ConnectionWriter::GetFreePackets()
{
    return std::move(_packets);
}

IPacket& ConnectionWriter::GetPacket()
{
    return **_packet;
}

IPacket::Ptr& ConnectionWriter::GetNextPacket(std::stop_token stop)
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
        _packet = previouseIterator;
        _packet++;
    }

    _offset = 0;
    assert(_packet != _packets.end());

    return *_packet;
}

IPacket::List ConnectionWriter::GetPacketToSend(std::stop_token stop)
{
    IPacket::List result;
    _packetsToSend.Wait(stop);
    _packetsToSend.LockedSwap(result);
    _packetsToSend.NotifyAll();
    return result;
}

void ConnectionWriter::SendThread(std::stop_token stop, ConnectionWriter& writer, IConnection& connection)
{
    while (!stop.stop_requested()) {
        IPacket::List packets = writer.GetPacketToSend(stop);
        IPacket::List freePackets = connection.Send(stop, std::move(packets));

        writer._freePackets.LockedSplice(std::move(freePackets));
        writer._freePackets.NotifyAll();
    }
}

} // namespace FastTransport::Protocol
