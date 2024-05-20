#pragma once

#include <filesystem>
#include <map>
#include <sys/types.h>

#include "File.hpp"

namespace FastTransport::FileSystem::FileCache {

class Range {
public:
    [[nodiscard]] off_t GetOffset() const;
    [[nodiscard]] size_t GetSize() const;
    [[nodiscard]] const FastTransport::Protocol::IPacket::List& GetPackets() const;
    [[nodiscard]] FastTransport::Protocol::IPacket::List& GetPackets();

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

private:
    off_t _offset;
    size_t _size;
    FastTransport::Protocol::IPacket::List _packets;
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
    std::map<Range, IPacket::List> _chunks;
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
