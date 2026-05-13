#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "MessageReader.hpp"
#include "ResponseFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {

class ResponseReadDirPlusJob : public ResponseFuseNetworkJob {
    using Reader = Protocol::MessageReader;

public:
    ResponseFuseNetworkJob::Message ExecuteResponse(std::stop_token stop, Writer& writer, FileTree& fileTree) override;

private:
    Reader reader;
};
} // namespace FastTransport::TaskQueue
