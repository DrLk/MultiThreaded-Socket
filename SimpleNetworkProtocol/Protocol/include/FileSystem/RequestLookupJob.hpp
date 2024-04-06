#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <string>

#include "FuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {
using Writer = Protocol::MessageWriter;

class RequestLookupJob : public FuseNetworkJob {
public:
    RequestLookupJob(fuse_req_t request, fuse_ino_t parent, std::string&& name);
    Message ExecuteMain(std::stop_token stop, Writer& writer) override;

private:
    fuse_req_t _request;
    fuse_ino_t _parent;
    std::string _name;
};
} // namespace FastTransport::TaskQueue
