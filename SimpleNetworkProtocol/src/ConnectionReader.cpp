#include "ConnectionReader.hpp"

#include <cstring>
#include <stop_token>
#include <utility>

#include "IConnection.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {

ConnectionReader::ConnectionReader(std::stop_token stop, const IConnection::Ptr& connection)
    : _connection(connection)
    , _stop(stop)
{
}

ConnectionReader& ConnectionReader::read(void* data, std::size_t size)
{
    if (_stop.stop_requested()) {
        return *this;
    }

    if (_packet == _packets.end()) {
        IPacket::List packets = _connection->Recv(_stop, std::move(_freePackets));
        _packets.splice(std::move(packets));
        _packet = _packets.begin();
    }

    if (_offset == GetPacket().GetPayload().size()) {
        IPacket::Ptr& nextPacket = GetNextPacket(_stop);
        if (!nextPacket) {
            _error = true;
            return *this;
        }
    }

    auto* bytes = reinterpret_cast<std::byte*>(data); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    while (size > 0) {
        auto readSize = std::min<std::uint32_t>(size, GetPacket().GetPayload().size() - _offset);
        std::memcpy(bytes, GetPacket().GetPayload().data() + _offset, readSize);
        _offset += readSize;

        size -= readSize;
        bytes += readSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    return *this;
}

ConnectionReader& ConnectionReader::operator>>(IPacket::List&& packets) // NOLINT(fuchsia-overloaded-operator)
{
    return *this;
}

IPacket& ConnectionReader::GetPacket()
{
    return **_packet;
}

IPacket::Ptr& ConnectionReader::GetNextPacket(std::stop_token stop)
{
    if (stop.stop_requested()) {
        static IPacket::Ptr null;
        return null;
    }

    auto previouseIterator = _packet;
    _packet++;
    if (_packet == _packets.end()) {
        IPacket::List packets = _connection->Recv(stop, std::move(_freePackets));
        if (packets.empty()) {
            static IPacket::Ptr null;
            return null;
        }
        _packets.splice(std::move(packets));
        _packet = previouseIterator;
        _packet++;
    }

    _offset = 0;
    assert(_packet != _packets.end());
    return *_packet;
}

/*void ConnectionReader::RecvThread(std::stop_token stop, ConnectionReader& reader, IConnection& connection)
{
    while (!stop.stop_requested()) {
        IPacket::List freePackets;
        reader._freePackets.LockedSwap(freePackets);
        IPacket::List packets = connection.Recv(stop, std::move(freePackets));
        reader._packetsToReceive.LockedSplice(std::move(packets));
    }
}*/

} // namespace FastTransport::Protocol
