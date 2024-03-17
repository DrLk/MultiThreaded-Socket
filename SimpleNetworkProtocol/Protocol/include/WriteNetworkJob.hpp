#pragma once

#include <memory>
#include <stop_token>

#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::Protocol {
class IConnection;
}

namespace FastTransport::TaskQueue {

class WriteNetworkJob : public Job {
public:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override; // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)

    virtual void ExecuteWriteNetwork(std::stop_token stop, ITaskScheduler& scheduler, Protocol::IConnection& connection) = 0;
};

} // namespace FastTransport::TaskQueue
