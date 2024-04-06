#pragma once

#include "MessageReader.hpp"
#include "ResponseFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {
using Writer = Protocol::MessageWriter;

class ResponseLookupJob : public ResponseFuseNetworkJob {
    using Reader = Protocol::MessageReader;
    using FileTree = FileSystem::FileTree;

public:
    Message ExecuteResponse(std::stop_token stop, Writer& writer, FileTree& fileTree) override;
};
} // namespace FastTransport::TaskQueue
