#include "ReadFileCacheJob.hpp"

#include "Logger.hpp"
#include "Packet.hpp"

#define TRACER() LOGGER() << "[ReadFileCacheJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileCache {

ReadFileCacheJob::ReadFileCacheJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _offset(offset)
    , _fileInfo(fileInfo)
{
}

void ReadFileCacheJob::ExecuteCachedTree(std::stop_token stop, FileTree& tree)
{
    TRACER() << "[read]"
             << " request: " << _request
             << " inode: " << _inode
             << " size: " << _size
             << " off: " << _offset
             << " fileInfo: " << _fileInfo;

    Protocol::IPacket::List data;
    for (int i = 0; i < 200; i++) {
        auto packet = std::make_unique<Protocol::Packet>(1500);
        std::array<std::byte, 1024> buffer {};
        packet->SetPayload(buffer);
        data.push_back(std::move(packet));
    }
    size_t readed = 0;
    readed = std::min(_size, _size);
    const std::size_t length = sizeof(fuse_bufvec) + sizeof(fuse_buf) * (data.size() - 1);
    std::unique_ptr<fuse_bufvec> buffVector(reinterpret_cast<fuse_bufvec*>(new char[length])); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    int index = 0;
    size_t count = 0;

    buffVector->off = 0;
    buffVector->idx = 0;
    for (auto& packet : data) {
        auto& buffer = buffVector->buf[index++]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        buffer.mem = packet->GetPayload().data();
        buffer.size = std::min(packet->GetPayload().size(), readed);
        buffer.flags = static_cast<fuse_buf_flags>(0); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        buffer.pos = 0;
        buffer.fd = 0;

        count++;
        if (readed <= packet->GetPayload().size()) {
            break;
        }

        readed -= packet->GetPayload().size();
        _offset += static_cast<off_t>(packet->GetPayload().size());
    }
    buffVector->count = count;

    fuse_reply_data(_request, buffVector.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
}

} // namespace FastTransport::FileCache
