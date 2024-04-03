#include "FuseReadFileJob.hpp"

#include "File.hpp"
#include "ITaskScheduler.hpp"
#include "ReadNetworkJob.hpp"

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
    _file.Read(freePackets, _size, _off);
    fuse_bufvec data = { 0 };
    fuse_reply_data(_request, &data, FUSE_BUF_FORCE_SPLICE);
    return freePackets;
}

} // namespace FastTransport::TaskQueue
