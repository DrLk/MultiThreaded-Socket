#pragma once

#include <memory>

#include "Job.hpp"

namespace FastTransport::Containers {
    template <class T>
    class MultiList;
} // namespace FastTransport::Containers
  //
namespace FastTransport::Protocol {
    class IPacket;
} // namespace FastTransport::Protocol

namespace FastTransport::TaskQueue {

class ITaskScheduler;

class DiskJob : public Job {
public:
    using Data = FastTransport::Containers::MultiList<std::unique_ptr<FastTransport::Protocol::IPacket>>;

    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;

    virtual Data ExecuteDisk(ITaskScheduler& scheduler, Data&& free) = 0;
};

} // namespace FastTransport::TaskQueue
