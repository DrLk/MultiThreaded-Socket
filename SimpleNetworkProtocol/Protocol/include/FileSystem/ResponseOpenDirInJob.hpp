#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"
#include "MessageReader.hpp"

namespace FastTransport::TaskQueue {

class ResponseOpenDirInJob : public FuseNetworkJob {
    using Reader = Protocol::MessageReader;
    using Writer = Protocol::MessageWriter;

public:
    Message ExecuteMain(std::stop_token stop, Writer& writer) override;
};
} // namespace FastTransport::TaskQueue
