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
};

} // namespace FastTransport::FileSystem
