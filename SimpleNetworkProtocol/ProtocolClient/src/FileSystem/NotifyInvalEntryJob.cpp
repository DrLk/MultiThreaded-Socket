#include "NotifyInvalEntryJob.hpp"

#include <cstdint>
#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <string>

#include "FileTree.hpp"
#include "ITaskScheduler.hpp"
#include "WatchEventType.hpp"
#include "Leaf.hpp"
#include "RemoteFileSystem.hpp"

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message NotifyInvalEntryJob::ExecuteResponse(ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& fileTree)
{
    auto& reader = GetReader();
    std::uint32_t eventTypeRaw = 0;
    std::uint64_t parentServerInode = 0;
    std::string name;
    std::uint64_t serverInode = 0;
    std::uint64_t size = 0;
    std::uint8_t isDirRaw = 0;
    reader >> eventTypeRaw;
    reader >> parentServerInode;
    reader >> name;
    reader >> serverInode;
    reader >> size;
    reader >> isDirRaw;

    const auto eventType = static_cast<FileSystem::WatchEventType>(eventTypeRaw);
    const bool isDir = isDirRaw != 0;

    const auto parentLeaf = fileTree.FindLeafByServerInode(parentServerInode);
    if (parentLeaf != nullptr) {
        if (eventType == FileSystem::WatchEventType::Created || eventType == FileSystem::WatchEventType::MovedTo) {
            const auto fileType = isDir ? std::filesystem::file_type::directory : std::filesystem::file_type::regular;
            FileSystem::Leaf& newLeaf = parentLeaf->AddChild(std::filesystem::path(name), fileType, static_cast<std::uintmax_t>(size));
            newLeaf.SetServerInode(serverInode);
            // Invalidate any negative dentry cached by the kernel for this name,
            // otherwise lookup will keep returning ENOENT until the TTL expires.
            fuse_session* const session = RemoteFileSystem::session;
            if (session != nullptr) {
                fuse_lowlevel_notify_inval_entry(session, GetINode(*parentLeaf), name.c_str(), name.size());
            }
        } else if (eventType == FileSystem::WatchEventType::Deleted || eventType == FileSystem::WatchEventType::MovedFrom) {
            parentLeaf->RemoveChild(name);
            fuse_session* const session = RemoteFileSystem::session;
            if (session != nullptr) {
                fuse_lowlevel_notify_inval_entry(session, GetINode(*parentLeaf), name.c_str(), name.size());
            }
        }
    }

    return GetFreeReadPackets();
}

} // namespace FastTransport::TaskQueue
