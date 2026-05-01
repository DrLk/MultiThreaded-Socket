#include "InotifyWatcher.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <poll.h>
#include <stdexcept>
#include <sys/inotify.h>
#include <unistd.h>
#include <vector>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[InotifyWatcher] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileSystem {

namespace {
    constexpr uint32_t WatchMask = IN_MODIFY | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB;
    constexpr int PollTimeoutMs = 100;
    constexpr size_t EventBufSize = 16U * (sizeof(inotify_event) + NAME_MAX + 1U);

    std::string ErrnoToString(int err)
    {
        std::array<char, 256> buf {};
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        return strerror_r(err, buf.data(), buf.size()) != nullptr ? buf.data() : "unknown error";
    }
} // namespace

InotifyWatcher::InotifyWatcher(std::filesystem::path root, WatchCallback callback)
    : _root(std::move(root))
    , _callback(std::move(callback))
    , _inotifyFd(inotify_init1(IN_NONBLOCK | IN_CLOEXEC))
{
    if (_inotifyFd < 0) {
        throw std::runtime_error(std::string("inotify_init1 failed: ") + ErrnoToString(errno));
    }
    AddWatch(_root);
}

InotifyWatcher::~InotifyWatcher()
{
    if (_inotifyFd >= 0) {
        close(_inotifyFd);
    }
}

void InotifyWatcher::AddWatch(const std::filesystem::path& startDir)
{
    // Iterative BFS to avoid recursion (misc-no-recursion).
    std::vector<std::filesystem::path> queue { startDir };
    while (!queue.empty()) {
        const std::filesystem::path dir = std::move(queue.back());
        queue.pop_back();

        const int watchDesc = inotify_add_watch(_inotifyFd, dir.c_str(), WatchMask);
        if (watchDesc < 0) {
            TRACER() << "inotify_add_watch failed for " << dir << ": " << ErrnoToString(errno);
            continue;
        }
        _wdToPath[watchDesc] = dir;
        _pathToWd[dir.string()] = watchDesc;

        std::error_code err;
        for (const auto& entry : std::filesystem::directory_iterator(dir, err)) {
            if (entry.is_directory(err)) {
                queue.push_back(entry.path());
            }
        }
    }
}

void InotifyWatcher::RemoveWatch(int watchDesc)
{
    auto watchIt = _wdToPath.find(watchDesc);
    if (watchIt == _wdToPath.end()) {
        return;
    }
    _pathToWd.erase(watchIt->second.string());
    _wdToPath.erase(watchIt);
    inotify_rm_watch(_inotifyFd, watchDesc);
}

void InotifyWatcher::ProcessEvent(const inotify_event* event)
{
    auto dirIt = _wdToPath.find(event->wd);
    if (dirIt == _wdToPath.end()) {
        return;
    }
    const std::filesystem::path& dir = dirIt->second;
    const bool isDir = (static_cast<uint32_t>(event->mask) & static_cast<uint32_t>(IN_ISDIR)) != 0U;
    std::filesystem::path name;
    if (event->len > 0) {
        name = event->name; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    }
    const std::filesystem::path fullPath = name.empty() ? dir : dir / name;

    if ((static_cast<uint32_t>(event->mask) & static_cast<uint32_t>(IN_CREATE)) != 0U && isDir) {
        AddWatch(fullPath);
    } else if ((static_cast<uint32_t>(event->mask) & static_cast<uint32_t>(IN_DELETE | IN_MOVED_FROM)) != 0U && isDir) {
        auto pdIt = _pathToWd.find(fullPath.string());
        if (pdIt != _pathToWd.end()) {
            RemoveWatch(pdIt->second);
        }
    }

    WatchEventType type = WatchEventType::Modified;
    if ((static_cast<uint32_t>(event->mask) & static_cast<uint32_t>(IN_MODIFY | IN_CLOSE_WRITE | IN_ATTRIB)) != 0U) {
        type = WatchEventType::Modified;
    } else if ((static_cast<uint32_t>(event->mask) & static_cast<uint32_t>(IN_CREATE)) != 0U) {
        type = WatchEventType::Created;
    } else if ((static_cast<uint32_t>(event->mask) & static_cast<uint32_t>(IN_DELETE)) != 0U) {
        type = WatchEventType::Deleted;
    } else if ((static_cast<uint32_t>(event->mask) & static_cast<uint32_t>(IN_MOVED_FROM)) != 0U) {
        type = WatchEventType::MovedFrom;
    } else if ((static_cast<uint32_t>(event->mask) & static_cast<uint32_t>(IN_MOVED_TO)) != 0U) {
        type = WatchEventType::MovedTo;
    } else {
        return;
    }

    _callback(WatchEvent { .path = fullPath, .name = name, .type = type, .isDir = isDir });
}

void InotifyWatcher::Run(std::stop_token stop)
{
    std::array<char, EventBufSize> buf {};
    pollfd pfd { .fd = _inotifyFd, .events = POLLIN, .revents = 0 };

    while (!stop.stop_requested()) {
        const int ready = poll(&pfd, 1, PollTimeoutMs);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            TRACER() << "poll failed: " << ErrnoToString(errno);
            break;
        }
        if (ready == 0) {
            continue;
        }

        const ssize_t length = read(_inotifyFd, buf.data(), buf.size());
        if (length < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            TRACER() << "read inotify failed: " << ErrnoToString(errno);
            break;
        }

        for (ssize_t idx = 0; idx < length;) {
            const auto* event = reinterpret_cast<const inotify_event*>(buf.data() + idx); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            ProcessEvent(event);
            idx += static_cast<ssize_t>(sizeof(inotify_event)) + event->len;
        }
    }
}

} // namespace FastTransport::FileSystem
