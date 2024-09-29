#include "ResponseReadFileJob.hpp"

#include <cassert>
#include <cstddef>
#include <stop_token>
#include <sys/uio.h>

#include "Logger.hpp"
#include "MessageType.hpp"
#include "RemoteFileHandle.hpp"
#include "ResponseFuseNetworkJob.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseReadFileJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& /*fileTree*/)
{

    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    size_t size = 0;
    off_t offset = 0;
    FileSystem::RemoteFileHandle* remoteFile = nullptr;
    auto& reader = GetReader();
    reader >> request;
    reader >> inode;
    reader >> size;
    reader >> offset;
    reader >> remoteFile;

    TRACER() << "Execute"
             << " request=" << request
             << " inode=" << inode
             << " size=" << size
             << " offset=" << offset
             << " remoteFile=" << remoteFile;

    writer << MessageType::ResponseRead;
    writer << request;
    writer << inode;
    writer << size;
    writer << offset;

    Message dataPackets = writer.GetDataPackets(1000);
    assert(size <= dataPackets.size() * dataPackets.front()->GetPayload().size());

    Message readed = remoteFile->file2->Read(dataPackets, dataPackets.size() * dataPackets.front()->GetPayload().size(), offset);
    int error = 0;
    if (readed.empty()) {
        TRACER() << "preadv failed";
        error = errno;
        writer << error;
        return {};
    }

    writer << error;
    writer << std::move(readed);

    return {};
}

} // namespace FastTransport::TaskQueue
