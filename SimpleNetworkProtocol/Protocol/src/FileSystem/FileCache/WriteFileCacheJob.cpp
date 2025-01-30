#include "WriteFileCacheJob.hpp"

#include "Logger.hpp"
#include "NativeFile.hpp"
#include "RemoteFileHandle.hpp"

#define TRACER() LOGGER() << "[WriteFileCacheJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileCache {
WriteFileCacheJob::WriteFileCacheJob(fuse_ino_t inode, size_t size, off_t offset, Message&& data, FileSystem::RemoteFileHandle* remoteFile)
    : _inode(inode)
    , _size(size)
    , _offset(offset)
    , _data(std::move(data))
    , _remoteFile(remoteFile)
{
}

WriteFileCacheJob::Message WriteFileCacheJob::ExecuteResponse(std::stop_token /*stop*/, FileTree& fileTree)
{
    TRACER() << "Execute"
             << " inode=" << _inode
             << " size=" << _size
             << " offset=" << _offset
             << " data.size=" << _data.size();

    Leaf& leaf = GetLeaf(_inode, fileTree);
    FileSystem::NativeFile file(leaf.GetCachePath());
    file.Write(_data, _size, _offset);
    return GetLeaf(_inode, fileTree).AddData(_offset, _size, std::move(_data));
}

} // namespace FastTransport::FileCache
