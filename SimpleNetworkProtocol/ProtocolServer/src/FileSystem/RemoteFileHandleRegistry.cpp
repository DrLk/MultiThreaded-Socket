#include "RemoteFileHandleRegistry.hpp"

#include <memory>
#include <mutex>
#include <utility>

namespace FastTransport::TaskQueue {

RemoteFileHandleRegistry& RemoteFileHandleRegistry::Instance()
{
    static RemoteFileHandleRegistry instance;
    return instance;
}

FileSystem::RemoteFileHandle* RemoteFileHandleRegistry::Register(std::shared_ptr<FileSystem::RemoteFileHandle> handle)
{
    auto* raw = handle.get();
    const std::lock_guard<std::mutex> lock(_mutex);
    _handles.emplace(raw, std::move(handle));
    return raw;
}

std::shared_ptr<FileSystem::RemoteFileHandle> RemoteFileHandleRegistry::Acquire(FileSystem::RemoteFileHandle* raw)
{
    const std::lock_guard<std::mutex> lock(_mutex);
    auto entry = _handles.find(raw);
    if (entry == _handles.end()) {
        return nullptr;
    }
    return entry->second;
}

std::shared_ptr<FileSystem::RemoteFileHandle> RemoteFileHandleRegistry::Take(FileSystem::RemoteFileHandle* raw)
{
    const std::lock_guard<std::mutex> lock(_mutex);
    auto entry = _handles.find(raw);
    if (entry == _handles.end()) {
        return nullptr;
    }
    auto owner = std::move(entry->second);
    _handles.erase(entry);
    return owner;
}

} // namespace FastTransport::TaskQueue
