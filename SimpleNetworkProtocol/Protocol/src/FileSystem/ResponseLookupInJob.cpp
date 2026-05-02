#include "ResponseLookupInJob.hpp"
#include <Tracy.hpp>

#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FileTree.hpp"
#include "FuseRequestTracker.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseLookupInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseLookupInJob::ExecuteResponse(ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& fileTree)
{
    ZoneScopedN("ResponseLookupInJob::ExecuteResponse");
    TRACER() << "Execute";

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t parentId = 0;
    std::string name;
    int error = 0;
    reader >> request;
    reader >> parentId;
    reader >> name;
    reader >> error;

    if (error != 0) {
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(request), error));
        return {};
    }

    fuse_entry_param entry {};
    reader >> entry.attr.st_ino;
    reader >> entry.attr.st_mode;
    reader >> entry.attr.st_nlink;
    reader >> entry.attr.st_size;
    reader >> entry.attr.st_uid;
    reader >> entry.attr.st_gid;
    reader >> entry.attr.st_mtim;

    entry.ino = entry.attr.st_ino;
    entry.attr_timeout = 1000.0;
    entry.entry_timeout = 1000.0;

    const std::filesystem::file_type type = S_ISDIR(entry.attr.st_mode) ? std::filesystem::file_type::directory : std::filesystem::file_type::regular;
    const uintmax_t size = entry.attr.st_size;

    FileSystem::Leaf* parentLeaf = parentId == FUSE_ROOT_ID
        ? &fileTree.GetRoot()
        : fileTree.FindLeafByServerInode(parentId);
    if (parentLeaf == nullptr) {
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(request), ENOENT));
        return {};
    }

    Leaf& newLeaf = parentLeaf->AddChild(name, type, size);
    newLeaf.SetServerInode(entry.attr.st_ino);
    entry.ino = reinterpret_cast<fuse_ino_t>(&newLeaf); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    entry.attr.st_ino = entry.ino;

    FUSE_ASSERT_REPLY(fuse_reply_entry(FUSE_UNTRACK(request), &entry));

    return {};
}

} // namespace FastTransport::TaskQueue
