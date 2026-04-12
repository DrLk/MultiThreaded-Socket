#include "MessageWriter.hpp"
#include <Tracy.hpp>

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
    ZoneScopedN("MessageWriter::operator<<(List)");
    operator<<(packets.size());

    IPacket::List freePackets;
    assert(_packet != _packets.end());

    _packet++;

    {
        ZoneScopedN("MessageWriter::SpliceTail");
        freePackets.splice(_packets, _packet, _packets.end());
    }

    _writedPacketNumber += packets.size();
    {
        ZoneScopedN("MessageWriter::SpliceData");
        _packets.splice(std::move(packets));
    }
    _packet = _packets.end();
    _packet--;
    {
        ZoneScopedN("MessageWriter::SpliceFree");
        _packets.splice(std::move(freePackets));
    }
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
    auto src = std::span(static_cast<const std::byte*>(data), size);
    while (!src.empty()) {
        auto& packet = GetPacket();
        if (_offset == packet.GetPayload().size()) {
            packet = GetNextPacket();
        }

        auto writeSize = std::min(src.size(), packet.GetPayload().size() - _offset);
        std::memcpy(packet.GetPayload().subspan(_offset).data(), src.data(), writeSize);
        _offset += writeSize;
        src = src.subspan(writeSize);
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
    ZoneScopedN("MessageWriter::GetDataPackets");
    IPacket::List freePackets;
    auto begin = _packet;
    assert(begin != _packets.end());
    begin++;
    {
        ZoneScopedN("GetDataPackets::SpliceTail");
        freePackets.splice(_packets, begin, _packets.end());
    }

    IPacket::List result;
    {
        ZoneScopedN("GetDataPackets::TryGenerate");
        result = freePackets.TryGenerate(size);
    }

    {
        ZoneScopedN("GetDataPackets::SpliceBack");
        _packets.splice(std::move(freePackets));
    }

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
