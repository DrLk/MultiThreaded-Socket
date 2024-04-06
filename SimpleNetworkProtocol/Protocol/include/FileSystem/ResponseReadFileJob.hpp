#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FuseNetworkJob.hpp"
#include "MessageReader.hpp"

namespace FastTransport::TaskQueue {

class ResponseReadFileJob : public FuseNetworkJob {
    using Reader = Protocol::MessageReader;

public:
    FuseNetworkJob::Message ExecuteMain(std::stop_token stop, Writer& writer) override;

private:
    Reader reader;
};
} // namespace FastTransport::TaskQueue
