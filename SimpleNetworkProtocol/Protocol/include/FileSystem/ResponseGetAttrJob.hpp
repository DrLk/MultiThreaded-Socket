#pragma once

#include "FuseNetworkJob.hpp"
#include "MessageReader.hpp"

namespace FastTransport::TaskQueue {
using Writer = Protocol::MessageWriter;

class ResponseGetAttrJob : public FuseNetworkJob {
    using Reader = Protocol::MessageReader;

public:
    Message ExecuteMain(std::stop_token stop, Writer& writer) override;
};
} // namespace FastTransport::TaskQueue
