#pragma once

#include "MainJob.hpp"

namespace FastTransport::TaskQueue {

class ITaskScheduler;

class MergeRequest : public MainJob {
public:
    static std::unique_ptr<MergeRequest> Create();

    MergeRequest();

    Message ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Message&& message) override;
};

} // namespace FastTransport::TaskQueue
