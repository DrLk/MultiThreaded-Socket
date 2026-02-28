#include "ReadFileCacheJob.hpp"

#include "FileCache/FuseReplyJob.hpp"
#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "NativeFile.hpp"
#include <fuse3/fuse_lowlevel.h>

namespace FastTransport::FileCache {

ReadFileCacheJob::ReadFileCacheJob(fuse_req_t request, FileSystem::NativeFile::Ptr& file, size_t size, off_t offset)
    : _request(request)
    , _file(file)
    , _size(size)
    , _offset(offset)
{
}

TaskQueue::DiskJob::Data ReadFileCacheJob::ExecuteDisk(TaskQueue::ITaskScheduler& scheduler, Protocol::IPacket::List&& free)
{
    Data packets = _file->Read(free, _size, _offset);
    scheduler.Schedule(std::make_unique<FuseReplyJob>(_request, std::move(packets)));
    return {};
}

} // namespace FastTransport::FileCache
