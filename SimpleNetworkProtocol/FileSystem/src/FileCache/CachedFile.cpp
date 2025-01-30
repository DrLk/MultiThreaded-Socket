#include "CachedFile.hpp"
#include <sys/stat.h>

#include "Range.hpp"

namespace FastTransport::FileSystem::FileCache {

CachedFile::CachedFile(std::filesystem::path&  /*path*/)
    : File(0, std::filesystem::file_type::regular)
{
}

CachedFile::~CachedFile()
= default;

void CachedFile::Open()
{
}

bool CachedFile::IsOpened() const
{
    return true;
}

int CachedFile::Close()
{
    return 0;
}

int CachedFile::Stat(struct stat&  /*stat*/)
{
    return 0;
}

CachedFile::IPacket::List CachedFile::Read(Protocol::IPacket::List&  /*packets*/, size_t  /*size*/, off_t  /*offset*/)
{
    return {};
}

void CachedFile::Write(Protocol::IPacket::List& /*packets*/, size_t /*size*/, off_t /*offset*/)
{
    throw std::runtime_error("Not implemented");
}

void CachedFile::Write(Protocol::IPacket::List&& packets, size_t size, off_t offset)
{
    _chunks.insert(Range(offset, size, std::move(packets)));
}

} // namespace FastTransport::FileSystem::FileCache
