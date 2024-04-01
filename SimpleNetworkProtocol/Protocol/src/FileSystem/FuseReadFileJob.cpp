#include "FuseReadFileJob.hpp"

#include "File.hpp"
#include "ITaskScheduler.hpp"

namespace FastTransport::TaskQueue {

FuseReadFileJob::FuseReadFileJob(File& file, fuse_req_t request, size_t size, off_t off)
    : _file(file)
    , _request(request)
    , _size(size)
    , _off(off)
{
}

Message FuseReadFileJob::ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Message&& freePackets)
{
    fuse_bufvec data = _file.Read(_size, _off);
    fuse_reply_data(_request, &data, FUSE_BUF_FORCE_SPLICE);
}

} // namespace FastTransport::TaskQueue
