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
    assert(_packet != _packets.end());
}

MessageReader& MessageReader::read(void* data, std::size_t size)
{
    auto dest = std::span(static_cast<std::byte*>(data), size);
    while (!dest.empty()) {
        auto& packet = GetPacket();
        if (_offset == packet.GetPayload().size()) {
            packet = GetNextPacket();
        }

        auto readSize = std::min(dest.size(), packet.GetPayload().size() - _offset);
        std::memcpy(dest.data(), packet.GetPayload().subspan(_offset).data(), readSize);
        _offset += readSize;
        dest = dest.subspan(readSize);
    }
    return *this;
}

MessageReader& MessageReader::operator>>(IPacket::List& packets) // NOLINT(fuchsia-overloaded-operator)
{
    std::size_t size = 0;
    operator>>(size);
    auto start = _packet;
    start++;
    auto end = start;
    for (std::size_t i = 0; i < size; i++) {
        if (end == _packets.end()) {
            break;
        }
        end++;
    }
    packets.splice(_packets, start, end);

    _offset = GetPacket().GetPayload().size();

    return *this;
}

MessageReader& MessageReader::operator>>(std::filesystem::path& path) // NOLINT(fuchsia-overloaded-operator)
{
    std::u8string string;
    operator>>(string);
    path = string;
    return *this;
}

IPacket::List MessageReader::GetPackets()
{
    return std::move(_packets);
}

IPacket::List MessageReader::GetFreePackets()
{
    IPacket::List freePackets;
    freePackets.splice(_packets, _packets.begin(), _packet);
    return freePackets;
}

IPacket& MessageReader::GetPacket()
{
    return **_packet;
}

IPacket& MessageReader::GetNextPacket()
{
    _packet++;
    assert(_packet != _packets.end());
    _offset = 0;

    return **_packet;
}

} // namespace FastTransport::Protocol
