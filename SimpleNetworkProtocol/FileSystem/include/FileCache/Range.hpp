#pragma once

#include <memory>

#include "MultiList.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {
class IPacket;
} // namespace FastTransport::Protocol

namespace FastTransport::FileSystem::FileCache {

class Range final {
public:
    using Data = Containers::MultiList<std::unique_ptr<Protocol::IPacket>>;
    Range(off_t offset, size_t size, Data&& data);
    Range(const Range&) = delete;
    Range(Range&& other) noexcept;
    Range& operator=(const Range&) = delete;
    Range& operator=(Range&& other) noexcept;

    ~Range();

    void SetOffset(off_t offset);
    [[nodiscard]] off_t GetOffset() const;
    void SetSize(size_t size);
    [[nodiscard]] size_t GetSize() const;

    [[nodiscard]] const Data& GetPackets() const;

    [[nodiscard]] Data& GetPackets();

    void AddRef();
    void Release();

    bool operator<(const Range& other) const; // NOLINT(fuchsia-overloaded-operator)

private:
    off_t _offset;
    size_t _size;
    Data _data;
    std::atomic<int> _counter { 1 };

};

} // namespace FastTransport::FileSystem::FileCache
