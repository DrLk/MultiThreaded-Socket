#include "InotifyWatcherJob.hpp"

#include <memory>
#include <stop_token>

#include "IServerTaskScheduler.hpp"
#include "ITaskScheduler.hpp"
#include "InotifyWatcher.hpp"
#include "Logger.hpp"
#include "ProcessInotifyEventJob.hpp"

#define TRACER() LOGGER() << "[InotifyWatcherJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

InotifyWatcherJob::InotifyWatcherJob(const std::filesystem::path& root, FileTree& fileTree, ITaskScheduler& scheduler)
    : _watcher(
          root,
          // The watcher invokes this callback on the inotify queue thread; we
          // must NOT touch FileTree here. Instead, marshal the event onto the
          // mainQueue (via ProcessInotifyEventJob) where all other server-side
          // tree mutations are serialized.
          [&fileTree, &scheduler](const FileSystem::WatchEvent& event) {
              scheduler.Schedule(std::make_unique<ProcessInotifyEventJob>(event, fileTree));
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
    auto* pointer = dynamic_cast<InotifyWatcherJob*>(job.get());
    if (pointer == nullptr) {
        return;
    }
    [[maybe_unused]] auto* released = job.release();
    dynamic_cast<IServerTaskScheduler&>(scheduler).ScheduleInotifyWatcherJob(std::unique_ptr<InotifyWatcherJob>(pointer));
}

} // namespace FastTransport::TaskQueue
