#include "SendNotifyInvalInodeJob.hpp"

#include <cstdint>
#include <stop_token>

#include "MessageType.hpp"
#include "ResponseFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {

SendNotifyInvalInodeJob::SendNotifyInvalInodeJob(std::uint64_t serverInode)
    : _serverInode(serverInode)
{
}

ResponseFuseNetworkJob::Message SendNotifyInvalInodeJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& /*fileTree*/)
{
    writer << MessageType::NotifyInvalInode;
    writer << _serverInode;
    return {};
}

} // namespace FastTransport::TaskQueue
