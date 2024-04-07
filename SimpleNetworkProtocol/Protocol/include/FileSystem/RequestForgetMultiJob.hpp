#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <span>

#include "FuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {
using Writer = Protocol::MessageWriter;

class RequestForgetMultiJob : public FuseNetworkJob {
public:
    RequestForgetMultiJob(fuse_req_t request, std::span<fuse_forget_data> forgets);
    Message ExecuteMain(std::stop_token stop, Writer& writer) override;

private:
    fuse_req_t _request;
    std::vector<fuse_forget_data> _forgets;
};
} // namespace FastTransport::TaskQueue
