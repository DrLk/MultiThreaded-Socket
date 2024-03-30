#pragma once

#include <functional>
#include <memory>
#include <stop_token>

#include "FileTree.hpp"
#include "MainJob.hpp"

namespace FastTransport::TaskQueue {

class ITaskScheduler;

class MergeOut : public MainJob {
public:
    static std::unique_ptr<MainJob> Create(FastTransport::FileSystem::FileTree& fileTree);

    MergeOut(FastTransport::FileSystem::FileTree& fileTree);

    Message ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Message&& message) override;

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
};

} // namespace FastTransport::TaskQueue
