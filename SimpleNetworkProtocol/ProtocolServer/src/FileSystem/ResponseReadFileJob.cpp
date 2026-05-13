#include "ResponseReadFileJob.hpp"
#include <Tracy.hpp>

#include <cassert>
#include <cstddef>
#include <memory>
#include <stop_token>
#include <sys/uio.h>

#include "IServerTaskScheduler.hpp"
#include "ITaskScheduler.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"
#include "RemoteFileHandle.hpp"
#include "ResponseFuseNetworkJob.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

void ResponseReadFileJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<ResponseReadFileJob*>(job.release());
    dynamic_cast<IServerTaskScheduler&>(scheduler).ScheduleResponseReadDiskJob(std::unique_ptr<ResponseReadFileJob>(pointer));
}

ResponseFuseNetworkJob::Message ResponseReadFileJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& /*fileTree*/)
{
    ZoneScopedN("ResponseReadFileJob::ExecuteResponse");
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    size_t size = 0;
    off_t offset = 0;
    off_t skipped = 0;
    FileSystem::RemoteFileHandle* remoteFile = nullptr;
    auto& reader = GetReader();
    reader >> request;
    reader >> inode;
    reader >> size;
    reader >> offset;
    reader >> skipped;
    reader >> remoteFile;

    TRACER() << "Execute"
             << " request=" << request
             << " inode=" << inode
             << " size=" << size
             << " offset=" << offset
             << " skipped=" << skipped
             << " remoteFile=" << remoteFile;

    writer << MessageType::ResponseRead;
    writer << request;
    writer << inode;
    writer << size;
    writer << offset;
    writer << skipped;

    Message dataPackets;
    {
        ZoneScopedN("ResponseReadFileJob::GetDataPackets");
        dataPackets = writer.GetDataPackets(1000);
    }
    assert(size <= dataPackets.size() * dataPackets.front()->GetPayload().size());

    Message readed = remoteFile->file2->Read(dataPackets, dataPackets.size() * dataPackets.front()->GetPayload().size(), offset + skipped);
    int error = 0;
    if (readed.empty()) {
        TRACER() << "preadv failed";
        error = errno;
        writer << error;
        return {};
    }

    writer << error;
    {
        ZoneScopedN("ResponseReadFileJob::WriteReaded");
        writer << std::move(readed);
    }

    return dataPackets;
}

} // namespace FastTransport::TaskQueue
