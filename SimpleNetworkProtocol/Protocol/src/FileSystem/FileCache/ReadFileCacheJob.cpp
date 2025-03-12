#include "ReadFileCacheJob.hpp"

#include <memory>

#include "ITaskScheduler.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"
#include "RequestReadFileJob.hpp"
#include "WriteFileCacheJob.hpp"

#define TRACER() LOGGER() << "[ReadFileCacheJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileCache {

ReadFileCacheJob::ReadFileCacheJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, FileSystem::RemoteFileHandle* remoteFile)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _offset(offset)
    , _remoteFile(remoteFile)
{
}

void ReadFileCacheJob::ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token /*stop*/, FileTree& tree)
{
    TRACER() << "[read]"
             << " request: " << _request
             << " inode: " << _inode
             << " size: " << _size
             << " offset: " << _offset
             << " remoteFile: " << _remoteFile;

    std::unique_ptr<fuse_bufvec> buffer = tree.GetData(_inode, _offset, _size);

    if (!buffer || buffer->count < 200) {
        if (_offset > Leaf::BlockSize) {
            auto [inode, offset, size, data] = tree.GetFreeData(1);
            if (data.size() > 0) {
                scheduler.Schedule(std::make_unique<FileCache::WriteFileCacheJob>(inode, _size, offset, std::move(data), _remoteFile));
            }
        }
        scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(_request, _inode, _size, _offset, _remoteFile));
        return;
    }

    fuse_reply_data(_request, buffer.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
}

} // namespace FastTransport::FileCache
