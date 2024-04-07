#include "ResponseForgetMultiJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>
#include <vector>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[ResponseForgetMultiJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseForgetMultiJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    std::vector<fuse_forget_data> forgets;
    reader >> request;
    reader >> forgets;

    TRACER() << "Execute"
             << " request: " << request;

    for (const auto& forget : forgets) {
        auto& leaf = GetLeaf(forget.ino, fileTree);
        leaf.ReleaseRef(forget.nlookup);
    }

    writer << MessageType::ResponseForgetMulti;
    writer << request;

    return {};
}

} // namespace FastTransport::TaskQueue
