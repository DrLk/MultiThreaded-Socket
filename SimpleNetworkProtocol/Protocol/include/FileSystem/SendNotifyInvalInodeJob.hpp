#pragma once

#include <cstdint>

#include "ResponseFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {

class SendNotifyInvalInodeJob : public ResponseFuseNetworkJob {
public:
    explicit SendNotifyInvalInodeJob(std::uint64_t serverInode);
    Message ExecuteResponse(std::stop_token stop, Writer& writer, FileTree& fileTree) override;

private:
    std::uint64_t _serverInode;
};

} // namespace FastTransport::TaskQueue
