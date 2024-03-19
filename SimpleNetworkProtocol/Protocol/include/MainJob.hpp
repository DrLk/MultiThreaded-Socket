#pragma once

#include <memory>
#include <stop_token>

#include "IPacket.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

class ITaskScheduler;

class MainJob : public Job {
protected:
    using Message = Protocol::IPacket::List;

public:
    [[nodiscard]] virtual Message ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Message&& freePackets) = 0;

private:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;
};

} // namespace FastTransport::TaskQueue
