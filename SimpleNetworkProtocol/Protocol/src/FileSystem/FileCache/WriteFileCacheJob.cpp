#include "WriteFileCacheJob.hpp"
#include <Tracy.hpp>

#include <memory>

#include "FileCache/FileCache.hpp"
#include "FreeRecvPacketsJob.hpp"
#include "ITaskScheduler.hpp"
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
    assert(!_data.empty());
}

WriteFileCacheJob::Message WriteFileCacheJob::ExecuteResponse(TaskQueue::ITaskScheduler& scheduler, std::stop_token /*stop*/, FileTree& fileTree)
{
    ZoneScopedN("WriteFileCacheJob::ExecuteResponse");
    TRACER() << "Execute"
             << " inode=" << _inode
             << " size=" << _size
             << " offset=" << _offset
             << " data.size=" << _data.size();

    Leaf& leaf = GetLeaf(_inode, fileTree);
    const FileSystem::NativeFile::Ptr file = fileTree.GetFileCache().GetFile(fileTree.GetCacheFolder() / leaf.GetCachePath());
    file->Write(_data, _size, _offset);
    assert(_size <= Leaf::BlockSize);
    leaf.GetPiecesStatus()->SetStatus(_offset / Leaf::BlockSize, FileSystem::PieceStatus::OnDisk);
    scheduler.ScheduleReadNetworkJob(std::make_unique<TaskQueue::FreeRecvPacketsJob>(std::move(_data)));
    return {};
}

} // namespace FastTransport::FileCache
