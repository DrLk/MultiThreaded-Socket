#include "MessageReader.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

#include "IPacket.hpp"

namespace FastTransport::Protocol {

MessageReader::MessageReader(IPacket::List&& packets)
    : _packets(std::move(packets))
    , _packet(_packets.begin())
{
}

MessageReader& MessageReader::read(std::byte* data, std::size_t size)
{
    if (_offset == GetPacket().GetPayload().size()) {
        GetNextPacket();
    }

    auto* bytes = data;
    while (size > 0) {
        auto readSize = std::min<std::uint32_t>(size, GetPacket().GetPayload().size() - _offset);
        std::memcpy(bytes, GetPacket().GetPayload().data() + _offset, readSize);
        _offset += readSize;

        size -= readSize;
        bytes += readSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    return *this;
}

MessageReader& MessageReader::operator>>(IPacket::List& /*packets*/) // NOLINT(fuchsia-overloaded-operator)
{
    return *this;
}

IPacket::List MessageReader::GetPackets()
{
    return std::move(_packets);
}

IPacket::List MessageReader::GetFreePackets()
{
    IPacket::List freePackets;
    freePackets.splice(std::move(_packets), _packets.begin(), _packet);
    return freePackets;
}

IPacket& MessageReader::GetPacket()
{
    return **_packet;
}

void MessageReader::GetNextPacket()
{
    _packet++;
    assert(_packet != _packets.end());
    _offset = 0;
}

} // namespace FastTransport::Protocol
