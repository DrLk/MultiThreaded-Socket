#include "MergeIn.hpp"

#include <memory>
#include <stop_token>
#include <utility>

#include "ByteStream.hpp"
#include "FileTree.hpp"
#include "FreeRecvPacketsJob.hpp"
#include "ITaskScheduler.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[MergeIn] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

std::unique_ptr<MergeIn> MergeIn::Create(FastTransport::FileSystem::FileTree& fileTree, MessageReader&& reader)
{
    return std::make_unique<MergeIn>(fileTree, std::move(reader));
}

MergeIn::MergeIn(FastTransport::FileSystem::FileTree& fileTree, MessageReader&& reader)
    : _fileTree(fileTree)
    , _reader(std::move(reader))
{
    TRACER() << "Create";
}

void MergeIn::ExecuteMainRead(std::stop_token stop, ITaskScheduler& scheduler)
{
    TRACER() << "Execute";

    Protocol::MessageReader reader(std::move(_reader));
    FileSystem::InputByteStream<Protocol::MessageReader> input(reader);

    _fileTree.get().Deserialize(input);

    scheduler.Schedule(FreeRecvPacketsJob::Create(_reader.GetPackets()));
}

} // namespace FastTransport::TaskQueue
