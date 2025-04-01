#include "ReadFileCacheJob.hpp"

#include "IPacket.hpp"
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

TaskQueue::DiskJob::Data ReadFileCacheJob::ExecuteDisk(TaskQueue::ITaskScheduler& /*scheduler*/, Protocol::IPacket::List&& free)
{
    //Data result = _file->Read(free, _size, _offset);

    fuse_bufvec buffer { .count = 1, .idx = 0, .off = 0 };
    buffer.buf[0].fd = _file->GetHandle();
    buffer.buf[0].flags = static_cast<fuse_buf_flags>(fuse_buf_flags::FUSE_BUF_IS_FD | fuse_buf_flags::FUSE_BUF_FD_SEEK); // NOLINT(hicpp-signed-bitwise)
    buffer.buf[0].pos = _offset;
    buffer.buf[0].size = _size;
    fuse_reply_data(_request, &buffer, fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);

    return std::move(free);
}

} // namespace FastTransport::FileCache
