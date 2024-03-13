#include "ConnectionWriter.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <stop_token>
#include <thread>
#include <utility>

#include "IConnection.hpp"
#include "IPacket.hpp"


namespace FastTransport::Protocol {

ConnectionWriter::ConnectionWriter(std::stop_token stop, const IConnection::Ptr& connection)
    : _connection(connection)
    , _stop(std::move(stop))
    , _sendThread(SendThread, std::ref(*this), std::ref(*connection))
{
    _packets = _connection->Send(_stop, IPacket::List());
    _packet = _packets.begin();
}

ConnectionWriter::~ConnectionWriter()
{
    flush();
}

ConnectionWriter& ConnectionWriter::operator<<(IPacket::List&& packets) // NOLINT(fuchsia-overloaded-operator)
{
    if (_stop.stop_requested()) {
        return *this;
    }

    flush();
    _packetsToSend.LockedSplice(std::move(packets));
    _packetsToSend.NotifyAll();
    return *this;
}

ConnectionWriter& ConnectionWriter::flush()
{
    if (_stop.stop_requested()) {
        return *this;
    }

    if (_packets.begin() != _packet) {
        IPacket::List packets;
        packets.splice(std::move(_packets), _packets.begin(), _packet);
        _packetsToSend.LockedSplice(std::move(packets));
    }

    auto packet = _packet;
    if (_offset > 0) {
        GetPacket().SetPayloadSize(_offset);
        _packetsToSend.LockedPushBack(std::move(*packet));
        GetNextPacket(_stop);
        _packets.erase(packet);
    }

    _packetsToSend.NotifyAll();
    _packetsToSend.WaitEmpty(_stop);

    return *this;
}

ConnectionWriter& ConnectionWriter::write(const void* data, std::size_t size)
{
    if (_stop.stop_requested()) {
        return *this;
    }

    const auto* bytes = reinterpret_cast<const std::byte*>(data); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    while (size > 0) {
        auto writeSize = std::min<std::uint32_t>(size, GetPacket().GetPayload().size() - _offset);
        std::memcpy(GetPacket().GetPayload().data() + _offset, bytes, writeSize);
        _offset += writeSize;

        size -= writeSize;
        bytes += writeSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        if (_offset == GetPacket().GetPayload().size()) {
            const IPacket::Ptr& nextPacket = GetNextPacket(_stop);
            if (!nextPacket) {
                _error = true;
                return *this;
            }
        }
    }

    return *this;
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

        if (packets.empty()) {
            continue;
        }

        IPacket::List freePackets = connection.Send(stop, std::move(packets));

        writer._freePackets.LockedSplice(std::move(freePackets));
        writer._freePackets.NotifyAll();
    }
}

} // namespace FastTransport::Protocol
