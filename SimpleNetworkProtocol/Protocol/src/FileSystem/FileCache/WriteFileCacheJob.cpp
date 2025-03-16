#include "WriteFileCacheJob.hpp"

#include "FileCache/FileCache.hpp"
#include "Logger.hpp"
#include "NativeFile.hpp"

#define TRACER() LOGGER() << "[WriteFileCacheJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileCache {
WriteFileCacheJob::WriteFileCacheJob(fuse_ino_t inode, size_t size, off_t offset, Message&& data)
    : _inode(inode)
    , _size(size)
    , _offset(offset)
    , _data(std::move(data))
{
    assert(_data.size() > 0);
}

WriteFileCacheJob::Message WriteFileCacheJob::ExecuteResponse(TaskQueue::ITaskScheduler&  /*scheduler*/, std::stop_token /*stop*/, FileTree& fileTree)
{
    TRACER() << "Execute"
             << " inode=" << _inode
             << " size=" << _size
             << " offset=" << _offset
             << " data.size=" << _data.size();

    Leaf& leaf = GetLeaf(_inode, fileTree);
    FileSystem::NativeFile::Ptr file = fileTree.GetFileCache().GetFile(fileTree.GetCacheFolder() / leaf.GetCachePath());
    file->Write(_data, _size, _offset);
    return std::move(_data);
}

} // namespace FastTransport::FileCache
