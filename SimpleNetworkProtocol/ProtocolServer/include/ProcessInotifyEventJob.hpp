#pragma once

#include <functional>
#include <stop_token>

#include "InotifyWatcher.hpp"
#include "MainReadJob.hpp"

namespace FastTransport::FileSystem {
class FileTree;
} // namespace FastTransport::FileSystem

namespace FastTransport::TaskQueue {

class ITaskScheduler;

// Carries an inotify event from the watcher thread to the mainQueue, where
// FileTree mutations are serialized with all other server-side tree operations
// (Rescan, AddChild from response handlers, etc.).
class ProcessInotifyEventJob : public MainReadJob {
public:
    ProcessInotifyEventJob(FileSystem::WatchEvent event, FileSystem::FileTree& fileTree);
    void ExecuteMainRead(std::stop_token stop, ITaskScheduler& scheduler) override;

private:
    FileSystem::WatchEvent _event;
    std::reference_wrapper<FileSystem::FileTree> _fileTree;
};

} // namespace FastTransport::TaskQueue
