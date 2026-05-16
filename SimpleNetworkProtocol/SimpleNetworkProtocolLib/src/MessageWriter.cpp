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
    // The writer treats every packet under its iterator (`_packet`) as if it
    // has the full MaxPayloadSize byte buffer to write into. Callers can hand
    // us packets that arrived with a shorter `payload_size` stored in the
    // header (e.g. the trailing short-read packet from a previous response was
    // returned to the free pool without a reset). Normalise here so the
    // invariant `_offset <= GetPayload().size()` holds for every packet we
    // iterate over.
    NormalizeCurrentPacket();
}

void MessageWriter::NormalizeCurrentPacket()
{
    if (_packet != _packets.end()) {
        (*_packet)->SetPayloadSize(MaxPayloadSize);
    }
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
    NormalizeCurrentPacket();

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

    // Callers (ResponseReadFileJob) compute available capacity as
    // `result.size() * result.front()->GetPayload().size()` and assert against
    // it. The header-stored payload size on a "free" packet should always be
    // MaxPayloadSize, but stale values can leak in from any path that returns
    // a packet to the send pool without resetting. Normalise here so the
    // contract — "data packets have MaxPayloadSize buffers" — holds.
    for (auto& packet : result) {
        packet->SetPayloadSize(MaxPayloadSize);
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
    NormalizeCurrentPacket();

    return **_packet;
}

} // namespace FastTransport::Protocol
