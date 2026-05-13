#pragma once

#include "MessageReader.hpp"
#include "ResponseFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {
using Writer = Protocol::MessageWriter;

class ResponseReleaseDirJob : public ResponseFuseNetworkJob {
    using Reader = Protocol::MessageReader;

public:
    Message ExecuteResponse(std::stop_token stop, Writer& writer, FileTree& fileTree) override;
};
} // namespace FastTransport::TaskQueue
