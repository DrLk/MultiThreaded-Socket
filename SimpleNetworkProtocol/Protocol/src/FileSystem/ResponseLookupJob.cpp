#include "ResponseLookupJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[ResponseLookupJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseLookupJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    TRACER() << "Execute";
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t parrentId = 0;
    std::string name;
    reader >> request;
    reader >> parrentId;
    reader >> name;

    TRACER() << "Execute"
             << " request: " << request;

    const Leaf& parrent = GetLeaf(parrentId, fileTree);
    auto file = parrent.Find(name);

    writer << MessageType::ResponseLookup;
    writer << request;
    writer << parrentId;
    writer << name;

    if (!file) {
        writer << ENOENT;
        return {};
    }

    const Leaf& fileRef = file.value().get();

    struct stat stbuf { };
    const std::string path = fileRef.GetFullPath();
    const int error = stat(path.c_str(), &stbuf);

    writer << error;

    if (error == 0) {

        fileRef.AddRef();
        writer << GetINode(file.value().get());
        writer << stbuf.st_mode;
        writer << stbuf.st_nlink;
        writer << stbuf.st_size;
        writer << stbuf.st_uid;
        writer << stbuf.st_gid;
        writer << stbuf.st_mtim;
    }

    return {};
}

} // namespace FastTransport::TaskQueue
