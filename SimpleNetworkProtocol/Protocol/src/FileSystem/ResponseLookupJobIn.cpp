#include "ResponseLookupJobIn.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/types.h>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseLookupJobIn] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

FuseNetworkJob::Message ResponseLookupJobIn::ExecuteMain(std::stop_token /*stop*/, Writer& /*writer*/)
{
    TRACER() << "Execute";

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    int error = 0;
    reader >> request;
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

    entry.ino = entry.attr.st_ino;
    entry.attr_timeout = 1.0;
    entry.entry_timeout = 1.0;

    fuse_reply_entry(request, &entry);

    return {};
}

} // namespace FastTransport::TaskQueue
