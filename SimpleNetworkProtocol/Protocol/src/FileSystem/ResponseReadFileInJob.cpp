#include "ResponseReadFileInJob.hpp"
#include <Tracy.hpp>

#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <stop_token>
#include <sys/types.h>

#include "ApplyBlockCacheJob.hpp"
#include "ITaskScheduler.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseReadFileInJob::ExecuteResponse(ITaskScheduler& scheduler, std::stop_token /*stop*/, FileTree& /*fileTree*/)
{
    ZoneScopedN("ResponseReadFileInJob::ExecuteResponse");
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    int error = 0;
    fuse_ino_t inode = 0;
    size_t size = 0;
    off_t offset = 0;
    off_t skipped = 0;
    reader >> request;
    reader >> inode;
    reader >> size;
    reader >> offset;
    reader >> skipped;
    reader >> error;

    TRACER() << "Execute"
             << " request=" << request
             << " inode=" << inode
             << " size=" << size
             << " offset=" << offset
             << " skipped=" << skipped
             << " error=" << error;

    if (error != 0) {
        if (request != nullptr) {
            fuse_reply_err(request, error);
        }
        return {};
    }

    Message data;
    reader >> data;

    // Hand the leaf/tree work off to the cacheTreeQueue so this mainQueue
    // thread is immediately free to process the next incoming response.
    scheduler.ScheduleCacheTreeJob(std::make_unique<ApplyBlockCacheJob>(
        request, inode, size, offset, skipped, std::move(data)));

    return {};
}

} // namespace FastTransport::TaskQueue
