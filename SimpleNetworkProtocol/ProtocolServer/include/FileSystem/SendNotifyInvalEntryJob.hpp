#pragma once

#include <cstdint>
#include <string>

#include "InotifyWatcher.hpp"
#include "ResponseFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {

class SendNotifyInvalEntryJob : public ResponseFuseNetworkJob {
public:
    SendNotifyInvalEntryJob(FileSystem::WatchEventType eventType, std::uint64_t parentServerInode, std::string name, std::uint64_t serverInode, std::uint64_t size, bool isDir);
    Message ExecuteResponse(std::stop_token stop, Writer& writer, FileTree& fileTree) override;

private:
    FileSystem::WatchEventType _eventType;
    std::uint64_t _parentServerInode;
    std::string _name;
    std::uint64_t _serverInode;
    std::uint64_t _size;
    bool _isDir;
};

} // namespace FastTransport::TaskQueue
