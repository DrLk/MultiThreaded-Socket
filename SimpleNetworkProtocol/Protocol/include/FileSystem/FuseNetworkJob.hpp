#pragma once

#include <memory>
#include <stop_token>

#include "IPacket.hpp"
#include "Job.hpp"
#include "MessageWriter.hpp"

namespace FastTransport::TaskQueue {

class ITaskScheduler;

class FuseNetworkJob : public Job {
protected:
    using Message = Protocol::IPacket::List;
    using Writer = Protocol::MessageWriter;

public:
    virtual Message ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Writer& writer) = 0;

private:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;
};

} // namespace FastTransport::TaskQueue
