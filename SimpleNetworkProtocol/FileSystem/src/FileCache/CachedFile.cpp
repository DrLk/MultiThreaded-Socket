#include "CachedFile.hpp"
#include <sys/stat.h>

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

std::size_t CachedFile::Read(Protocol::IPacket::List&  /*packets*/, size_t  /*size*/, off_t  /*offset*/)
{
    return 0;
}

void CachedFile::Write(Protocol::IPacket::List&  /*packets*/, size_t  size, off_t  offset)
{
    _chunks.insert(std::make_pair(Range{offset, size}, Protocol::IPacket::List{}));
}


} // namespace FastTransport::FileSystem::FileCache
