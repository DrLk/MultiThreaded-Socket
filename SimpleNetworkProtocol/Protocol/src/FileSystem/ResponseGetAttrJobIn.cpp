#include "ResponseGetAttrJobIn.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/types.h>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileJobIn] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

FuseNetworkJob::Message ResponseGetAttrJobIn::ExecuteMain(std::stop_token /*stop*/, Writer&  /*writer*/)
{

    auto& reader = GetReader();
    fuse_req_t _request = nullptr;
    int error = 0;
    reader >> _request;
    reader >> error;

    TRACER() << "Execute"
             << " request: " << _request;

    if (error != 0) {
        fuse_reply_err(_request, error);
        return {};
    }

    struct stat stbuf { };
    reader >> stbuf.st_ino;
    reader >> stbuf.st_mode;
    reader >> stbuf.st_nlink;
    reader >> stbuf.st_size;

    fuse_reply_attr(_request, &stbuf, 1.0);

    return reader.GetPackets();
}

} // namespace FastTransport::TaskQueue
