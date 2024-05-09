#include "ResponseLookupJob.hpp"

#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "Leaf.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"
#include "NativeFile.hpp"

#define TRACER() LOGGER() << "[ResponseLookupJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

static FileSystem::Leaf& AddLeaf(const std::filesystem::path& path, FileSystem::Leaf& parent)
{
    auto type = std::filesystem::status(path).type();
    if (type == std::filesystem::file_type::not_found) {
        return parent.AddChild(path.filename(), type);
    }
    auto size = type == std::filesystem::file_type::directory ? 0 : std::filesystem::file_size(path);
    return parent.AddFile(path.filename(), std::unique_ptr<FileSystem::File>(new FileSystem::NativeFile(path, size, type)));
}

ResponseFuseNetworkJob::Message ResponseLookupJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    TRACER() << "Execute";
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t parentId = 0;
    std::string name;
    reader >> request;
    reader >> parentId;
    reader >> name;

    TRACER() << "Execute"
             << " request: " << request;

    Leaf& parent = GetLeaf(parentId, fileTree);
    auto file = parent.Find(name);

    writer << MessageType::ResponseLookup;
    writer << request;
    writer << parentId;
    writer << name;

    const std::string path = parent.GetFullPath() / name;
    const Leaf& fileRef = file ? file.value().get() : AddLeaf(path, parent);

    if (fileRef.IsDeleted()) {
        writer << ENOENT;
        return {};
    }

    struct stat stbuf { };
    const int error = stat(path.c_str(), &stbuf);

    writer << error;

    if (error == 0) {

        fileRef.AddRef();
        writer << GetINode(fileRef);
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
