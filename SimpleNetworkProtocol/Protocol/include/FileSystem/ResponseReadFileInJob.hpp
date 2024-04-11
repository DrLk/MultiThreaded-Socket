#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "MessageReader.hpp"
#include "ResponseInFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {

class ResponseReadFileInJob : public ResponseInFuseNetworkJob {
    using Reader = Protocol::MessageReader;

public:
    Message ExecuteResponse(std::stop_token stop, FileTree& fileTree) override;

private:
    Reader _reader;
    void GetBuffer(const Message& message);
};
} // namespace FastTransport::TaskQueue
