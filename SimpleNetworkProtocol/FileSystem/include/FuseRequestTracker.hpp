#pragma once

#ifndef NDEBUG

#include <cassert>
#include <fuse3/fuse_lowlevel.h>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "Logger.hpp"

namespace FastTransport::FileSystem {

// Tracks all active (unreplied) fuse_req_t requests for debug purposes.
// Call FUSE_TRACK when a request arrives and FUSE_UNTRACK before each fuse_reply_*.
// Call FUSE_DUMP_PENDING to log all requests that were never replied to.
class FuseRequestTracker {
public:
    static FuseRequestTracker& Instance()
    {
        static FuseRequestTracker instance; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
        return instance;
    }

    FuseRequestTracker(const FuseRequestTracker&) = delete;
    FuseRequestTracker(FuseRequestTracker&&) = delete;
    FuseRequestTracker& operator=(const FuseRequestTracker&) = delete;
    FuseRequestTracker& operator=(FuseRequestTracker&&) = delete;
    ~FuseRequestTracker() = default;

    void Track(fuse_req_t req, std::string_view operation)
    {
        const std::scoped_lock lock(_mutex);
        _pending.emplace(req, std::string(operation));
    }

    fuse_req_t Untrack(fuse_req_t req)
    {
        const std::scoped_lock lock(_mutex);
        _pending.erase(req);
        return req;
    }

    void DumpPending() const
    {
        const std::scoped_lock lock(_mutex);
        if (_pending.empty()) {
            LOGGER() << "[FuseRequestTracker] No pending unreplied requests";
            return;
        }
        LOGGER() << "[FuseRequestTracker] " << _pending.size() << " unreplied request(s) at session exit:";
        for (const auto& [req, operation] : _pending) {
            LOGGER() << "[FuseRequestTracker]   req=" << req << " op=" << operation;
        }
    }

private:
    FuseRequestTracker() = default;
    mutable std::mutex _mutex;
    std::unordered_map<fuse_req_t, std::string> _pending;
};

// Returns result unchanged; asserts it equals 0 (success).
// fuse_reply_none is void and cannot fail — use it directly without this wrapper.
inline int FuseReplyAssert(int result)
{
    assert(result == 0);
    return result;
}

// Like FuseReplyAssert but tolerates -ENOENT (kernel freed the request because
// the requesting process was killed). Use only in Cancel()/destructor shutdown paths.
inline int FuseReplyShutdown(int result)
{
    assert(result == 0 || result == -ENOENT);
    return result;
}

} // namespace FastTransport::FileSystem

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define FUSE_TRACK(req, op) FastTransport::FileSystem::FuseRequestTracker::Instance().Track((req), (op))
#define FUSE_UNTRACK(req) FastTransport::FileSystem::FuseRequestTracker::Instance().Untrack(req)
#define FUSE_DUMP_PENDING() FastTransport::FileSystem::FuseRequestTracker::Instance().DumpPending()
#define FUSE_ASSERT_REPLY(call) FastTransport::FileSystem::FuseReplyAssert(call)
#define FUSE_REPLY_SHUTDOWN(call) FastTransport::FileSystem::FuseReplyShutdown(call)
// NOLINTEND(cppcoreguidelines-macro-usage)

#else

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define FUSE_TRACK(req, op) ((void)0)
#define FUSE_UNTRACK(req) (req)
#define FUSE_DUMP_PENDING() ((void)0)
#define FUSE_ASSERT_REPLY(call) (call)
#define FUSE_REPLY_SHUTDOWN(call) (call)
// NOLINTEND(cppcoreguidelines-macro-usage)

#endif
