#pragma once

#include "DiskJob.hpp"
#include "IPacket.hpp"

namespace FastTransport::FileSystem {
class File;
} // namespace FastTransport::FileSystem

namespace FastTransport::TaskQueue {

class OpenFileJob : public DiskJob {

    using Message = Protocol::IPacket::List;
    using File = FileSystem::File;

public:
    OpenFileJob(File& file);
    virtual void ExecuteDisk(ITaskScheduler& scheduler, float disk) override;
private:
    File& _file;
};
} // namespace FastTransport::TaskQueue
