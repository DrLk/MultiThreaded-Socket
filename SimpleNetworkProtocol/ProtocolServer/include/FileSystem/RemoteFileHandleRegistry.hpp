#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include "RemoteFileHandle.hpp"

namespace FastTransport::TaskQueue {

// Server-side ownership registry for RemoteFileHandle.
//
// The wire protocol identifies an open file/dir by the raw RemoteFileHandle*
// the server allocated in ResponseOpenJob and echoed back to the client.
// The hazard: read jobs hop to the disk thread, while release jobs run on
// the main queue. If a Release happens before a queued Read reaches the
// disk thread, the handle is freed and the disk thread dereferences a
// dangling pointer (heap-use-after-free reported by TSAN in
// ResponseReadFileJob.cpp:68).
//
// The registry keeps a shared_ptr per live handle. Read jobs Acquire a
// shared_ptr copy on the disk thread; Release jobs Take the registry's
// owning copy. If a Read still holds a copy, the handle survives until the
// Read finishes — no UAF, and the wire-level raw pointer keeps working as
// an opaque ID.
class RemoteFileHandleRegistry {
public:
    static RemoteFileHandleRegistry& Instance();

    // Inserts a new handle and returns the raw pointer used as the wire ID.
    FileSystem::RemoteFileHandle* Register(std::shared_ptr<FileSystem::RemoteFileHandle> handle);

    // Returns the shared_ptr for `raw` if still registered, or nullptr if it
    // has already been Take-n by a Release. Call this on the worker thread
    // that is about to dereference the handle.
    std::shared_ptr<FileSystem::RemoteFileHandle> Acquire(FileSystem::RemoteFileHandle* raw);

    // Removes `raw` from the registry and returns the registry's owning
    // shared_ptr (which may still be aliased by an in-flight Acquire'd
    // copy). Callers can drop the returned shared_ptr to allow destruction
    // once all in-flight references are gone.
    std::shared_ptr<FileSystem::RemoteFileHandle> Take(FileSystem::RemoteFileHandle* raw);

private:
    RemoteFileHandleRegistry() = default;

    std::mutex _mutex;
    std::unordered_map<FileSystem::RemoteFileHandle*, std::shared_ptr<FileSystem::RemoteFileHandle>> _handles;
};

} // namespace FastTransport::TaskQueue
