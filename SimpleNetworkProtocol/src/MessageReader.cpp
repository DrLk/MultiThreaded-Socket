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

MessageReader& MessageReader::read(void* data, std::size_t size)
{
    if (_offset == GetPacket().GetPayload().size()) {
        GetNextPacket();
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

MessageReader& MessageReader::operator>>(IPacket::List&& /*packets*/) // NOLINT(fuchsia-overloaded-operator)
{
    return *this;
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
