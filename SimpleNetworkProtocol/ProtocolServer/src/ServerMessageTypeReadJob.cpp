#include "ServerMessageTypeReadJob.hpp"

#include <memory>

#include "ITaskScheduler.hpp"
#include "MessageReader.hpp"
#include "MessageType.hpp"

#include "FileSystem/ResponseForgetMultiJob.hpp"
#include "FileSystem/ResponseGetAttrJob.hpp"
#include "FileSystem/ResponseLookupJob.hpp"
#include "FileSystem/ResponseOpenDirJob.hpp"
#include "FileSystem/ResponseOpenJob.hpp"
#include "FileSystem/ResponseReadDirJob.hpp"
#include "FileSystem/ResponseReadDirPlusJob.hpp"
#include "FileSystem/ResponseReadFileJob.hpp"
#include "FileSystem/ResponseReleaseDirJob.hpp"
#include "FileSystem/ResponseReleaseJob.hpp"

namespace FastTransport::TaskQueue {

std::unique_ptr<ReadNetworkJob> ServerMessageTypeReadJob::Create(FileTree& fileTree, Message&& messages)
{
    return std::make_unique<ServerMessageTypeReadJob>(fileTree, std::move(messages));
}

std::unique_ptr<ReadNetworkJob> ServerMessageTypeReadJob::CreateNext(Message&& messages)
{
    return Create(GetFileTree(), std::move(messages));
}

bool ServerMessageTypeReadJob::DispatchSideSpecific(MessageType type, Protocol::MessageReader&& reader, ITaskScheduler& scheduler)
{
    switch (type) {
    case MessageType::RequestGetAttr: {
        auto job = std::make_unique<ResponseGetAttrJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::RequestLookup: {
        auto job = std::make_unique<ResponseLookupJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::RequestOpen: {
        auto job = std::make_unique<ResponseOpenJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::RequestOpenDir: {
        auto job = std::make_unique<ResponseOpenDirJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::RequestForgetMulti: {
        auto job = std::make_unique<ResponseForgetMultiJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::RequestRelease: {
        auto job = std::make_unique<ResponseReleaseJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::RequestReleaseDir: {
        auto job = std::make_unique<ResponseReleaseDirJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::RequestRead: {
        auto job = std::make_unique<ResponseReadFileJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::RequestReadDir: {
        auto job = std::make_unique<ResponseReadDirJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    case MessageType::RequestReadDirPlus: {
        auto job = std::make_unique<ResponseReadDirPlusJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        return true;
    }
    default:
        return false;
    }
}

} // namespace FastTransport::TaskQueue
