#include "ResponseGetAttrJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

FuseNetworkJob::Message ResponseGetAttrJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute";

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    int file = 0;
    reader >> request;
    reader >> inode;
    reader >> file;

    writer << MessageType::ResponseGetAttr;
    writer << request;

    int error = 0;

    if (inode == FUSE_ROOT_ID) {
        writer << error;
        writer << inode;
        writer << (S_IFDIR | 0755);
        writer << 2;
        writer << 0;
        return {};
    }

    struct stat stbuf { };
    error = fstat(file, &stbuf);

    writer << error;

    if (error == 0) {
        writer << inode;
        writer << stbuf.st_mode;
        writer << stbuf.st_nlink;
        writer << stbuf.st_size;
    }

    return {};
}

} // namespace FastTransport::TaskQueue
