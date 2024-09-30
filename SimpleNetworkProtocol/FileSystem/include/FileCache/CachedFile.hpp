#pragma once

#include <filesystem>
#include <set>
#include <sys/types.h>

#include "File.hpp"
#include "Range.hpp"

namespace FastTransport::FileSystem::FileCache {

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
    IPacket::List Read(IPacket::List& packets, size_t size, off_t offset) override;
    void Write(Protocol::IPacket::List&& packets, size_t size, off_t offset) override;

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
