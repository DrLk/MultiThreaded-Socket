#pragma once

#include <filesystem>
#include <functional>
#include <stop_token>
#include <sys/inotify.h>
#include <unordered_map>

#include "WatchEventType.hpp"

namespace FastTransport::FileSystem {

struct WatchEvent {
    std::filesystem::path path;
    std::filesystem::path name;
    WatchEventType type;
    bool isDir;
} __attribute__((aligned(128)));

using WatchCallback = std::function<void(const WatchEvent&)>;

class InotifyWatcher {
public:
    explicit InotifyWatcher(std::filesystem::path root, WatchCallback callback);
    ~InotifyWatcher();

    InotifyWatcher(const InotifyWatcher&) = delete;
    InotifyWatcher& operator=(const InotifyWatcher&) = delete;
    InotifyWatcher(InotifyWatcher&&) = delete;
    InotifyWatcher& operator=(InotifyWatcher&&) = delete;

    void Run(std::stop_token stop);

private:
    void AddWatch(const std::filesystem::path& dir);
    void RemoveWatch(int watchDesc);
    void ProcessEvent(const struct inotify_event* event);

    std::filesystem::path _root;
    WatchCallback _callback;
    int _inotifyFd = -1;
    int _stopFd = -1;
    std::unordered_map<int, std::filesystem::path> _wdToPath;
    std::unordered_map<std::string, int> _pathToWd;
};

} // namespace FastTransport::FileSystem
