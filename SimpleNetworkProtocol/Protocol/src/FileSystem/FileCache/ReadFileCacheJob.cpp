#include "ReadFileCacheJob.hpp"

#include "ITaskScheduler.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"
#include "RequestReadFileJob.hpp"

#define TRACER() LOGGER() << "[ReadFileCacheJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileCache {

ReadFileCacheJob::ReadFileCacheJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _offset(offset)
    , _fileInfo(fileInfo)
{
}

void ReadFileCacheJob::ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token /*stop*/, FileTree& tree)
{
    TRACER() << "[read]"
             << " request: " << _request
             << " inode: " << _inode
             << " size: " << _size
             << " off: " << _offset
             << " fileInfo: " << _fileInfo;

    auto& leaf = GetLeaf(_inode, tree);
    std::unique_ptr<fuse_bufvec> buffer = leaf.GetData(_offset, _size);

    if (!buffer) {
        scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(_request, _inode, _size, _offset, _fileInfo));
        return;
    }
    fuse_reply_data(_request, buffer.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
}

} // namespace FastTransport::FileCache
