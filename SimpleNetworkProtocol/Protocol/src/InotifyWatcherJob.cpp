#include "InotifyWatcherJob.hpp"

#include <filesystem>
#include <memory>
#include <stop_token>
#include <system_error>
#include <utility>

#include "FileSystem/SendNotifyInvalEntryJob.hpp"
#include "FileSystem/SendNotifyInvalInodeJob.hpp"
#include "ITaskScheduler.hpp"
#include "InotifyWatcher.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[InotifyWatcherJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

namespace {

    FileSystem::Leaf* FindLeafByPath(FileSystem::Leaf& node, const std::filesystem::path& target) // NOLINT(misc-no-recursion)
    {
        if (node.GetFullPath() == target) {
            return &node;
        }
        for (const auto& [name, child] : node.GetChildren()) {
            if (auto* const found = FindLeafByPath(const_cast<FileSystem::Leaf&>(child), target)) { // NOLINT(cppcoreguidelines-pro-type-const-cast)
                return found;
            }
        }
        return nullptr;
    }

} // namespace

InotifyWatcherJob::InotifyWatcherJob(const std::filesystem::path& root, FileTree& fileTree, ITaskScheduler& scheduler)
    : _watcher(
          root,
          [root, &fileTree, &scheduler](const FileSystem::WatchEvent& event) {
              switch (event.type) {
              case FileSystem::WatchEventType::Modified: {
                  const FileSystem::Leaf* const leaf = FindLeafByPath(fileTree.GetRoot(), event.path);
                  if (leaf != nullptr) {
                      TRACER() << "Modified: " << event.path;
                      const std::uint64_t serverInode = leaf->GetServerInode();
                      scheduler.Schedule(std::make_unique<SendNotifyInvalInodeJob>(serverInode));
                  }
                  break;
              }
              case FileSystem::WatchEventType::Created:
              case FileSystem::WatchEventType::MovedTo: {
                  const std::filesystem::path parentPath = event.path.parent_path();
                  FileSystem::Leaf* const parentLeaf = FindLeafByPath(fileTree.GetRoot(), parentPath);
                  if (parentLeaf != nullptr) {
                      TRACER() << "Created: " << event.path;
                      std::error_code errCode;
                      const auto fileType = event.isDir ? std::filesystem::file_type::directory : std::filesystem::file_type::regular;
                      const std::uintmax_t size = event.isDir ? 0 : std::filesystem::file_size(event.path, errCode);
                      const FileSystem::Leaf& newLeaf = parentLeaf->AddChild(std::filesystem::path(event.name), fileType, errCode ? 0 : size);
                      const std::uint64_t newServerInode = newLeaf.GetServerInode();
                      const std::uint64_t parentServerInode = parentLeaf->GetServerInode();
                      scheduler.Schedule(std::make_unique<SendNotifyInvalEntryJob>(
                          event.type, parentServerInode, event.name.string(), newServerInode, errCode ? 0 : size, event.isDir));
                      scheduler.Schedule(std::make_unique<SendNotifyInvalInodeJob>(parentServerInode));
                  }
                  break;
              }
              case FileSystem::WatchEventType::Deleted:
              case FileSystem::WatchEventType::MovedFrom: {
                  const std::filesystem::path parentPath = event.path.parent_path();
                  FileSystem::Leaf* const parentLeaf = FindLeafByPath(fileTree.GetRoot(), parentPath);
                  if (parentLeaf != nullptr) {
                      TRACER() << "Deleted: " << event.path;
                      const std::uint64_t parentServerInode = parentLeaf->GetServerInode();
                      parentLeaf->RemoveChild(event.name.string());
                      scheduler.Schedule(std::make_unique<SendNotifyInvalEntryJob>(
                          event.type, parentServerInode, event.name.string(), 0, 0, false));
                      scheduler.Schedule(std::make_unique<SendNotifyInvalInodeJob>(parentServerInode));
                  }
                  break;
              }
              }
          })
    , _fileTree(fileTree)
    , _scheduler(scheduler)
{
}

void InotifyWatcherJob::ExecuteMainRead(std::stop_token stop, ITaskScheduler& /*scheduler*/)
{
    TRACER() << "Starting inotify watch loop";
    _watcher.Run(stop);
    TRACER() << "Inotify watch loop stopped";
}

void InotifyWatcherJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<InotifyWatcherJob*>(job.release());
    std::unique_ptr<InotifyWatcherJob> watcherJob(pointer);
    scheduler.ScheduleInotifyWatcherJob(std::move(watcherJob));
}

} // namespace FastTransport::TaskQueue
