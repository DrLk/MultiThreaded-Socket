#include "RemoteFileHandleRegistry.hpp"

#include <memory>
#include <mutex>
#include <utility>

namespace FastTransport::FileSystem {

RemoteFileHandle* RemoteFileHandleRegistry::Register(std::shared_ptr<RemoteFileHandle> handle)
{
    auto* raw = handle.get();
    const std::scoped_lock lock(_mutex);
    _handles.emplace(raw, std::move(handle));
    return raw;
}

std::shared_ptr<RemoteFileHandle> RemoteFileHandleRegistry::Acquire(RemoteFileHandle* raw)
{
    const std::scoped_lock lock(_mutex);
    auto entry = _handles.find(raw);
    if (entry == _handles.end()) {
        return nullptr;
    }
    return entry->second;
}

std::shared_ptr<RemoteFileHandle> RemoteFileHandleRegistry::Take(RemoteFileHandle* raw)
{
    const std::scoped_lock lock(_mutex);
    auto entry = _handles.find(raw);
    if (entry == _handles.end()) {
        return nullptr;
    }
    auto owner = std::move(entry->second);
    _handles.erase(entry);
    return owner;
}

} // namespace FastTransport::FileSystem
