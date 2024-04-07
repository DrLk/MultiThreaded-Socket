#pragma once

#include "MessageReader.hpp"
#include "ResponseFuseNetworkJob.hpp"

namespace FastTransport::FileSystem {
class FileTree;
} // namespace FastTransport::FileSystem

namespace FastTransport::TaskQueue {
using Writer = Protocol::MessageWriter;
using FileTree = FileSystem::FileTree;

class ResponseForgetMultiJob : public ResponseFuseNetworkJob {
    using Reader = Protocol::MessageReader;

public:
    Message ExecuteResponse(std::stop_token stop, Writer& writer, FileTree& fileTree) override;
};
} // namespace FastTransport::TaskQueue
