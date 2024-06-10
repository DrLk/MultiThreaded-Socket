#pragma once

#include <filesystem>
#include <set>
#include <sys/types.h>

#include "File.hpp"

namespace FastTransport::FileSystem::FileCache {

class Range final {
public:
    Range(size_t size, off_t offset)
        : _offset(offset)
        , _size(size)
    {
    }

    Range(const Range&) = delete;
    Range(Range&& other) noexcept
        : _offset(other._offset)
        , _size(other._size)
        , _data(std::move(other._data))
    {
    }

    Range& operator=(const Range&) = delete;
    Range& operator=(Range&& other) noexcept
    {
        _offset = other._offset;
        _size = other._size;
        _data = std::move(other._data);
        return *this;
    }

    ~Range() = default;

    [[nodiscard]] off_t GetOffset() const
    {
        return _offset;
    }

    [[nodiscard]] size_t GetSize() const
    {
        return _size;
    }

    [[nodiscard]] const FastTransport::Protocol::IPacket::List& GetPackets() const
    {
        return _data;
    }
    
    [[nodiscard]] FastTransport::Protocol::IPacket::List& GetPackets()
    {
        return _data;
    }

    void AddRef()
    {
        _counter++;
    }

    void Release()
    {
        if (--_counter == 0) {
            delete this;
        }
    }

    bool operator<(const Range& other) const // NOLINT(fuchsia-overloaded-operator)
    {
        if (_offset < other._offset) {
            return true;
        }

        return _offset == other._offset && _size < other._size;
    }

private:
    off_t _offset;
    size_t _size;
    FastTransport::Protocol::IPacket::List _data;
    std::atomic<int> _counter { 1 };

} __attribute__((aligned(64)));

class CachedFile : public File {
private:
    using IPacket = FastTransport::Protocol::IPacket;

public:
    explicit CachedFile(std::filesystem::path& path);
    CachedFile(const CachedFile&) = delete;
    CachedFile(CachedFile&&) = delete;
    CachedFile& operator=(const CachedFile&) = delete;
    CachedFile& operator=(CachedFile&&) = delete;
    ~CachedFile() override;

    void Open() override;
    [[nodiscard]] bool IsOpened() const override;
    int Close() override;
    int Stat(struct stat& stat) override;
    std::size_t Read(IPacket::List& packets, size_t size, off_t offset) override;
    void Write(Protocol::IPacket::List& packets, size_t size, off_t offset) override;

private:
    std::set<Range> _chunks;
};

class WriteCachedFileJob {
public:
    WriteCachedFileJob(CachedFile* file, Range* range);

    void ExecuteWrite();
    void ExecuteCachedTree();

private:
    CachedFile* _file {};
    Range* _range {};
};

} // namespace FastTransport::FileSystem::FileCache
