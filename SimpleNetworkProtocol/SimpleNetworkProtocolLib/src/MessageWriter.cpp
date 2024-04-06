#include "MessageWriter.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <span>
#include <utility>

#include "IPacket.hpp"

namespace FastTransport::Protocol {

MessageWriter::MessageWriter(IPacket::List&& packets)
    : _packets(std::move(packets))
    , _packet(_packets.begin())
{
}

MessageWriter::MessageWriter(MessageWriter&&) noexcept = default;

MessageWriter::~MessageWriter() = default;

MessageWriter& MessageWriter::operator=(MessageWriter&&) noexcept = default;

MessageWriter& MessageWriter::operator<<(IPacket::List&& packets) // NOLINT(fuchsia-overloaded-operator)
{
    _packets.splice(std::move(packets));
    return *this;
}

MessageWriter& MessageWriter::operator<<(const std::filesystem::path& path) // NOLINT(fuchsia-overloaded-operator)
{
    operator<<(path.u8string());
    return *this;
}

MessageWriter& MessageWriter::write(const void* data, std::size_t size)
{
    const auto* bytes = reinterpret_cast<const std::byte*>(data); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    while (size > 0) {
        if (_offset == GetPacket().GetPayload().size()) {
            GetNextPacket();
        }

        auto writeSize = std::min<std::uint32_t>(size, GetPacket().GetPayload().size() - _offset);
        std::memcpy(GetPacket().GetPayload().data() + _offset, bytes, writeSize);
        _offset += writeSize;

        size -= writeSize;
        bytes += writeSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    return *this;
}

MessageWriter& MessageWriter::flush()
{
    return *this;
}

IPacket::List MessageWriter::GetPackets()
{
    return std::move(_packets);
}

IPacket::List MessageWriter::GetWritedPackets()
{
    IPacket::List writedPackets;
    if (_offset == 0 && _packet != _packets.begin()) {
        return writedPackets;
    }

    if (_offset == sizeof(int) && _packet == _packets.begin()) {
        return writedPackets;
    }

    assert(_packet != _packets.end());
    std::memcpy(_packets.front()->GetPayload().data(), &_writedPacketNumber, sizeof(_writedPacketNumber));
    _packet++;
    writedPackets.splice(_packets, _packets.begin(), _packet);
    return writedPackets;
}

IPacket& MessageWriter::GetPacket()
{
    return **_packet;
}

void MessageWriter::GetNextPacket()
{
    _writedPacketNumber++;
    _packet++;
    assert(_packet != _packets.end());

    _offset = 0;
}

} // namespace FastTransport::Protocol
