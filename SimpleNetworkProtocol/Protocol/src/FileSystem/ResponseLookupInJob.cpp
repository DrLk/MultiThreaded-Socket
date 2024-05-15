#include "ResponseLookupInJob.hpp"

#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseLookupInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseLookupInJob::ExecuteResponse(std::stop_token /*stop*/, FileTree& fileTree)
{
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
        fuse_reply_err(request, error);
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
    GetLeaf(parentId, fileTree).AddChild(name, type);

    fuse_reply_entry(request, &entry);

    return {};
}

} // namespace FastTransport::TaskQueue
