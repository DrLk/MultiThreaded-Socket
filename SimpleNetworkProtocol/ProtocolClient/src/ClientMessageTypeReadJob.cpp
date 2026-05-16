#include "ClientMessageTypeReadJob.hpp"

#include <memory>

#include "ITaskScheduler.hpp"
#include "MessageReader.hpp"
#include "MessageType.hpp"

#include "FileSystem/NotifyInvalEntryJob.hpp"
#include "FileSystem/NotifyInvalInodeJob.hpp"
#include "FileSystem/ResponseForgetMultiInJob.hpp"
#include "FileSystem/ResponseGetAttrInJob.hpp"
#include "FileSystem/ResponseLookupInJob.hpp"
#include "FileSystem/ResponseOpenDirInJob.hpp"
#include "FileSystem/ResponseOpenInJob.hpp"
#include "FileSystem/ResponseReadDirInJob.hpp"
#include "FileSystem/ResponseReadDirPlusInJob.hpp"
#include "FileSystem/ResponseReadFileInJob.hpp"
#include "FileSystem/ResponseReleaseDirInJob.hpp"
#include "FileSystem/ResponseReleaseInJob.hpp"

namespace FastTransport::TaskQueue {

std::unique_ptr<ReadNetworkJob> ClientMessageTypeReadJob::Create(FileTree& fileTree, Message&& messages)
{
    return std::make_unique<ClientMessageTypeReadJob>(fileTree, std::move(messages));
}

std::unique_ptr<ReadNetworkJob> ClientMessageTypeReadJob::CreateNext(Message&& messages)
{
    return Create(GetFileTree(), std::move(messages));
}

bool ClientMessageTypeReadJob::DispatchSideSpecific(MessageType type, Protocol::MessageReader&& reader, ITaskScheduler& scheduler)
{
    switch (type) {
    case MessageType::ResponseGetAttr: {
        auto job = std::make_unique<ResponseGetAttrInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::ResponseLookup: {
        auto job = std::make_unique<ResponseLookupInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::ResponseOpen: {
        auto job = std::make_unique<ResponseOpenInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::ResponseOpenDir: {
        auto job = std::make_unique<ResponseOpenDirInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::ResponseForgetMulti: {
        auto job = std::make_unique<ResponseForgetMultiInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::ResponseRelease: {
        auto job = std::make_unique<ResponseReleaseInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::ResponseReleaseDir: {
        auto job = std::make_unique<ResponseReleaseDirInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::ResponseRead: {
        auto job = std::make_unique<ResponseReadFileInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::ResponseReadDir: {
        auto job = std::make_unique<ResponseReadDirInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::ResponseReadDirPlus: {
        auto job = std::make_unique<ResponseReadDirPlusInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::NotifyInvalInode: {
        auto job = std::make_unique<NotifyInvalInodeJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::NotifyInvalEntry: {
        auto job = std::make_unique<NotifyInvalEntryJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    default:
        return false;
    }
}

} // namespace FastTransport::TaskQueue
