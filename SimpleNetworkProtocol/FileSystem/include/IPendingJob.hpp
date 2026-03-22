#pragma once

namespace FastTransport::FileSystem {

class IPendingJob {
public:
    virtual ~IPendingJob() = default;
    virtual void Execute() = 0;
};

} // namespace FastTransport::FileSystem
