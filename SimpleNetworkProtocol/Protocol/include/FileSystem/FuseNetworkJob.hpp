#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <stop_token>

#include "FileHandle.hpp"
#include "IPacket.hpp"
#include "Job.hpp"
#include "MessageReader.hpp"
#include "MessageWriter.hpp"

namespace FastTransport::TaskQueue {

class FuseNetworkJob : public Job {
protected:
    using Message = Protocol::IPacket::List;
    using Writer = Protocol::MessageWriter;
    using Reader = Protocol::MessageReader;
    using FileHandle = FileSystem::FileHandle;

public:
    virtual Message ExecuteMain(std::stop_token stop, Writer& writer) = 0;

    void InitReader(Reader&& reader);
    Reader& GetReader();
    Message GetFreeReadPackets();
    FileHandle& GetFileHandle(fuse_file_info* fileInfo);

private:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;

    Reader _reader;
};

} // namespace FastTransport::TaskQueue
