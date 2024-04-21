#include "MessageWriter.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <span>
#include <sys/stat.h>
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
    operator<<(packets.size());

    IPacket::List freePackets;
    assert(_packet != _packets.end());

    _packet++;

    freePackets.splice(_packets, _packet, _packets.end());

    _writedPacketNumber += packets.size();
    _packets.splice(std::move(packets));
    _packet = _packets.end();
    _packet--;
    _packets.splice(std::move(freePackets));
    _packet++;

    _offset = 0;

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
        auto& packet = GetPacket();
        auto payload = packet.GetPayload();
        if (_offset == payload.size()) {
            packet = GetNextPacket();
        }

        auto writeSize = std::min<std::uint32_t>(size, packet.GetPayload().size() - _offset);
        std::memcpy(packet.GetPayload().data() + _offset, bytes, writeSize);
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

    if (_packet == _packets.begin() && _offset == sizeof(int)) {
        return writedPackets;
    }

    if (_offset == 0 && _packet != _packets.begin()) {
        _packet--;
    }

    assert(_packet != _packets.end());
    std::memcpy(_packets.front()->GetPayload().data(), &_writedPacketNumber, sizeof(_writedPacketNumber));
    _packet++;
    writedPackets.splice(_packets, _packets.begin(), _packet);
    return writedPackets;
}

IPacket::List MessageWriter::GetDataPackets(std::size_t size)
{
    IPacket::List freePackets;
    auto begin = _packet;
    assert(begin != _packets.end());
    begin++;
    freePackets.splice(_packets, begin, _packets.end());

    IPacket::List result = freePackets.TryGenerate(size);

    _packets.splice(std::move(freePackets));

    return result;
}
void MessageWriter::AddFreePackets(IPacket::List&& freePackets)
{
    _packets.splice(std::move(freePackets));
}

IPacket& MessageWriter::GetPacket()
{
    return **_packet;
}

IPacket& MessageWriter::GetNextPacket()
{
    _writedPacketNumber++;
    _packet++;
    assert(_packet != _packets.end());

    _offset = 0;

    return **_packet;
}

} // namespace FastTransport::Protocol
