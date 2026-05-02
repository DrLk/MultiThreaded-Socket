#include "SendNotifyInvalEntryJob.hpp"

#include <cstdint>
#include <stop_token>
#include <string>
#include <utility>

#include "InotifyWatcher.hpp"
#include "MessageType.hpp"
#include "ResponseFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {

SendNotifyInvalEntryJob::SendNotifyInvalEntryJob(FileSystem::WatchEventType eventType, std::uint64_t parentServerInode, std::string name, std::uint64_t serverInode, std::uint64_t size, bool isDir)
    : _eventType(eventType)
    , _parentServerInode(parentServerInode)
    , _name(std::move(name))
    , _serverInode(serverInode)
    , _size(size)
    , _isDir(isDir)
{
}

ResponseFuseNetworkJob::Message SendNotifyInvalEntryJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& /*fileTree*/)
{
    const auto eventType = static_cast<std::uint32_t>(_eventType);
    const auto isDir = static_cast<std::uint8_t>(_isDir ? 1 : 0);
    writer << MessageType::NotifyInvalEntry;
    writer << eventType;
    writer << _parentServerInode;
    writer << _name;
    writer << _serverInode;
    writer << _size;
    writer << isDir;
    return {};
}

} // namespace FastTransport::TaskQueue
