#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <stop_token>

#include "FileTree.hpp"
#include "InotifyWatcher.hpp"
#include "MainReadJob.hpp"

namespace FastTransport::TaskQueue {

class ITaskScheduler;

class InotifyWatcherJob : public MainReadJob {
    using FileTree = FileSystem::FileTree;

public:
    InotifyWatcherJob(const std::filesystem::path& root, FileTree& fileTree, ITaskScheduler& scheduler);

    void ExecuteMainRead(std::stop_token stop, ITaskScheduler& scheduler) override;
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;

private:
    FileSystem::InotifyWatcher _watcher;
    std::reference_wrapper<FileTree> _fileTree;
    std::reference_wrapper<ITaskScheduler> _scheduler;
};

} // namespace FastTransport::TaskQueue
