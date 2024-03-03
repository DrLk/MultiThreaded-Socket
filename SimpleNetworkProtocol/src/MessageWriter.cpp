#include "MessageWriter.hpp"

#include "IConnection.hpp"
#include "IPacket.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace FastTransport::Protocol {

MessageWriter::MessageWriter(IPacket::List&& packets)
    : _packets(std::move(packets))
    , _packet(_packets.begin())
{
    operator<<(static_cast<int>(_packets.size()));
}

MessageWriter::MessageWriter(MessageWriter&&) = default;

MessageWriter::~MessageWriter() = default;

MessageWriter& MessageWriter::operator=(MessageWriter&&) = default;

MessageWriter& MessageWriter::operator<<(IPacket::List&& packets) // NOLINT(fuchsia-overloaded-operator)
{
    _packets.splice(std::move(packets));
    return *this;
}

MessageWriter& MessageWriter::write(const void* data, std::size_t size)
{
    const auto* bytes = reinterpret_cast<const std::byte*>(data); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    while (size > 0) {
        auto writeSize = std::min<std::uint32_t>(size, GetPacket().GetPayload().size() - _offset);
        std::memcpy(GetPacket().GetPayload().data() + _offset, bytes, writeSize);
        _offset += writeSize;

        size -= writeSize;
        bytes += writeSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        if (_offset == GetPacket().GetPayload().size()) {
            GetNextPacket();
        }
    }

    return *this;
}

IPacket::List MessageWriter::GetPackets()
{
    return std::move(_packets);
}

IPacket& MessageWriter::GetPacket()
{
    return **_packet;
}

void MessageWriter::GetNextPacket()
{
    _packet++;
    if (_packet == _packets.end()) {
        throw std::runtime_error("No more free packets");
    }

    _offset = 0;
    assert(_packet != _packets.end());
}

} // namespace FastTransport::Protocol
