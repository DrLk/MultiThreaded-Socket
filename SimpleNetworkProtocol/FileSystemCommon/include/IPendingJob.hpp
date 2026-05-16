#pragma once

namespace FastTransport::FileSystem {

class IPendingJob {
public:
    IPendingJob() = default;
    IPendingJob(const IPendingJob&) = delete;
    IPendingJob& operator=(const IPendingJob&) = delete;
    IPendingJob(IPendingJob&&) = delete;
    IPendingJob& operator=(IPendingJob&&) = delete;
    virtual ~IPendingJob() = default;
    virtual void Execute() = 0;
    // Called instead of Execute() when the scheduler is shutting down and the
    // awaited block will never arrive. Implementations should send an error reply
    // to any held fuse_req_t so the FUSE kernel can unblock the calling process.
    virtual void Cancel() { }
};

} // namespace FastTransport::FileSystem
