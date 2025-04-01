#include "FuseReadFileJob.hpp"

#include <memory>

#include "FileCache/FileCache.hpp"
#include "ITaskScheduler.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"
#include "NativeFile.hpp"
#include "ReadFileCacheJob.hpp"
#include "RequestReadFileJob.hpp"

#define TRACER() LOGGER() << "[ReadFileCacheJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileCache {

FuseReadFileJob::FuseReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, FileSystem::RemoteFileHandle* remoteFile)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _offset(offset)
    , _remoteFile(remoteFile)
{
}

void FuseReadFileJob::ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token /*stop*/, FileTree& tree)
{
    TRACER() << "[read]"
             << " request: " << _request
             << " inode: " << _inode
             << " size: " << _size
             << " offset: " << _offset
             << " remoteFile: " << _remoteFile;

    auto& leaf = _inode == FUSE_ROOT_ID ? tree.GetRoot() : *(reinterpret_cast<Leaf*>(_inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

    if (leaf.GetSize() <= _offset) {
        TRACER() << "File size is less than offset";
        fuse_reply_err(_request, EIO);
        return;
    }

    if (leaf.GetSize() < _offset + _size) {
        TRACER() << "Last block is not full";
        _size = leaf.GetSize() - _offset;
    }

    std::unique_ptr<fuse_bufvec> buffer = tree.GetData(_inode, _offset, _size);

    if (!buffer || buffer->count == 0) {
        off_t skipped = _offset > 0 ? Leaf::BlockSize - (_offset % Leaf::BlockSize) : 0;
        if (_offset + skipped > leaf.GetSize()) {
            skipped -= Leaf::BlockSize;
        }
        FileSystem::NativeFile::Ptr file = tree.GetFileCache().GetFile(tree.GetCacheFolder() / leaf.GetCachePath());
        Protocol::IPacket::List data;
        scheduler.Schedule(std::make_unique<ReadFileCacheJob>(_request, file, _size, _offset));
        scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(_request, _inode, _size, _offset, skipped, _remoteFile));
        return;
    }

    fuse_reply_data(_request, buffer.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
}

} // namespace FastTransport::FileCache
