#include "MergeOut.hpp"

#include <functional>
#include <memory>
#include <utility>

#include "FileTree.hpp"
#include "ITaskScheduler.hpp"
#include "LeafSerializer.hpp"
#include "Logger.hpp"
#include "MainJob.hpp"
#include "MessageType.hpp"
#include "MessageWriter.hpp"
#include "SendMessageJob.hpp"

#define TRACER() LOGGER() << "[MergeOout] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

std::unique_ptr<MainJob> MergeOut::Create(FastTransport::FileSystem::FileTree& fileTree)
{
    return std::make_unique<MergeOut>(fileTree);
}

MergeOut::MergeOut(FastTransport::FileSystem::FileTree& fileTree)
    : _fileTree(fileTree)
{
    TRACER() << "Create";
}

MergeOut::Message MergeOut::ExecuteMain(std::stop_token  /*stop*/, ITaskScheduler& scheduler, Message&& message)
{
    TRACER() << "Execute";

    Protocol::MessageWriter writer(std::move(message));
    writer << MessageType::ResponseTree;
    FileSystem::OutputByteStream<Protocol::MessageWriter> output(writer);
    FileSystem::LeafSerializer::Serialize(_fileTree.get().GetRoot(), output);

    scheduler.Schedule(SendMessageJob::Create(writer.GetWritedPackets()));
    return writer.GetPackets();
}

} // namespace FastTransport::TaskQueue
