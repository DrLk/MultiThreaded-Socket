#include "ResponseReadDirPlusInJob.hpp"
#include <Tracy.hpp>

#include <bit>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <linux/fuse.h>
#include <stop_token>
#include <sys/stat.h>

#include "FileCache/PinnedFuseBufVec.hpp"
#include "FuseRequestTracker.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReadDirPlusJobIn] " // NOLINT(cppcoreguidelines-macro-usage)

namespace {

fuse_ino_t ResolveClientInode(std::string_view name, fuse_ino_t parentInode, FastTransport::FileSystem::Leaf& parentLeaf, const ::fuse_direntplus& entry)
{
    if (name == ".") {
        return parentInode;
    }
    if (name == "..") {
        const FastTransport::FileSystem::Leaf* grandparent = parentLeaf.GetParent();
        return (grandparent == nullptr)
            ? FUSE_ROOT_ID
            : reinterpret_cast<fuse_ino_t>(grandparent); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    }
    // readdirplus may be issued repeatedly for the same directory. Reuse the
    // existing Leaf if we have already learned about this name — otherwise the
    // kernel-visible inode (= Leaf*) would change between readdirplus calls and
    // invalidate previously handed-out references.
    const std::string nameStr(name);
    auto childLeaf = parentLeaf.FindChild(nameStr);
    FastTransport::FileSystem::Leaf* childPtr = childLeaf.get();
    if (childPtr == nullptr) {
        const auto type = (S_ISDIR(entry.entry_out.attr.mode) != 0)
            ? std::filesystem::file_type::directory
            : std::filesystem::file_type::regular;
        childPtr = &parentLeaf.AddChild(std::filesystem::path(nameStr), type, entry.entry_out.attr.size);
        childPtr->SetServerInode(entry.entry_out.nodeid);
    }
    // Each direntplus entry given to the kernel increments its nlookup; track
    // that locally so the Leaf survives until the matching forget arrives.
    childPtr->AddRef();
    return reinterpret_cast<fuse_ino_t>(childPtr); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

// WARNING: This walker decodes the kernel FUSE wire layout (struct fuse_direntplus
// from <linux/fuse.h>). The server constructs these buffers via fuse_add_direntry_plus
// from libfuse, which encodes the same layout. The two sides therefore stay in
// sync only as long as both are built against compatible libfuse/kernel headers.
// If libfuse ever switches to a non-public encoding, this code will silently
// produce garbage. Keep this parser the only place that touches the raw layout.
void PopulateTreeAndPatchInodes(fuse_ino_t parentInode, FastTransport::FileSystem::Leaf& parentLeaf, std::span<std::byte> payload)
{
    constexpr size_t kNameOff = offsetof(::fuse_direntplus, dirent.name);
    size_t pos = 0;
    while (pos < payload.size()) {
        auto entrySpan = payload.subspan(pos);
        if (entrySpan.size() < sizeof(::fuse_direntplus)) {
            break;
        }
        // The payload buffer has only byte alignment, so direct field access
        // through a fuse_direntplus* would be UB. Memcpy through an aligned
        // local copy to read namelen / patch inode fields.
        ::fuse_direntplus entry {};
        std::memcpy(&entry, entrySpan.data(), sizeof(entry));
        const uint32_t namelen = entry.dirent.namelen;
        if (namelen == 0 || entrySpan.size() < kNameOff + namelen) {
            break;
        }
        const auto nameSpan = entrySpan.subspan(kNameOff, namelen);
        const std::string_view name(reinterpret_cast<const char*>(nameSpan.data()), nameSpan.size()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

        const fuse_ino_t localIno = ResolveClientInode(name, parentInode, parentLeaf, entry);
        entry.entry_out.nodeid = localIno;
        entry.entry_out.attr.ino = localIno;
        entry.dirent.ino = localIno;
        std::memcpy(entrySpan.data(), &entry, sizeof(entry));

        pos += (kNameOff + namelen + (sizeof(uint64_t) - 1)) & ~(sizeof(uint64_t) - 1);
    }
}

} // namespace

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseReadDirPlusInJob::ExecuteResponse(ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& fileTree)
{
    ZoneScopedN("ResponseReadDirPlusInJob::ExecuteResponse");
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t clientInode = 0;
    Message data;

    reader >> request;
    reader >> clientInode;

    TRACER() << "Execute"
             << " request: " << request;

    reader >> data;

    auto buffVector = FileSystem::FileCache::AllocateFuseBufVec(data.size());
    buffVector->off = 0;
    buffVector->idx = 0;
    buffVector->count = data.size();

    Leaf& parentLeaf = GetLeaf(clientInode, fileTree);

    int index = 0;
    for (auto& packet : data) {
        auto payload = packet->GetPayload();

        auto& buffer = buffVector->buf[index++]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        buffer.mem = payload.data();
        buffer.size = payload.size();
        buffer.flags = std::bit_cast<fuse_buf_flags>(0);
        buffer.pos = 0;

        PopulateTreeAndPatchInodes(clientInode, parentLeaf, payload);
    }

    FUSE_ASSERT_REPLY(fuse_reply_data(FUSE_UNTRACK(request), buffVector.get(), fuse_buf_copy_flags::FUSE_BUF_SPLICE_MOVE));

    return data;
}

} // namespace FastTransport::TaskQueue
