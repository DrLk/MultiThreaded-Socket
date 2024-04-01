#pragma once

#include <stop_token>
#include <fuse3/fuse_lowlevel.h>

#include "DiskJob.hpp"
#include "FuseNetworkJob.hpp"
#include "MainReadJob.hpp"
#include "MessageReader.hpp"

namespace FastTransport::TaskQueue {

class ResponseReadFileJob : public FuseNetworkJob {
    using Reader = Protocol::MessageReader;

public:
    ResponseReadFileJob(Reader&& reader);
    FuseNetworkJob::Message ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Writer& writer) override;

private:
    Reader _reader;
};
} // namespace FastTransport::TaskQueue
