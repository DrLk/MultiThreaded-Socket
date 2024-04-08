#pragma once

#include <cstdint>
#include <functional>
#include <sys/types.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fuse3/fuse_lowlevel.h>

#include "File.hpp"
#include "FileTree.hpp"

namespace FastTransport::FileSystem {
class FileSystem {
    using FilePtr = std::unique_ptr<File>;

public:
    explicit FileSystem(std::string_view mountpoint);
    void Start();

protected:
    std::function<void(fuse_req_t, fuse_ino_t, fuse_file_info*)> _getattr;
    std::function<void(fuse_req_t, fuse_ino_t, const char*)> _lookup;
    std::function<void(fuse_req_t, fuse_ino_t, fuse_file_info*)> _opendir;
    std::function<void(fuse_req_t, fuse_ino_t, uint64_t)> _forget;
    std::function<void(fuse_req_t, size_t, fuse_forget_data*)> _forgetMulti;
    std::function<void(fuse_req_t, fuse_ino_t, fuse_file_info*)> _release;

private:
    struct dirbuf {
        char* p;
        size_t size;
    } __attribute__((aligned(16)));

    static void FuseInit(void* userdata, struct fuse_conn_info* conn);
    static void BufferAddFile(fuse_req_t req, struct dirbuf* buffer, const char* name, fuse_ino_t ino);
    static int ReplyBufferLimited(fuse_req_t req, const char* buffer, size_t bufferSize, off_t off, size_t maxSize);
    static void Stat(fuse_ino_t ino, struct stat* stbuf, const File& file);

    static void FuseLookup(fuse_req_t req, fuse_ino_t parentId, const char* name);
    static void FuseForget(fuse_req_t req, fuse_ino_t inode, uint64_t nlookup);
    static void FuseGetattr(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseReaddir(fuse_req_t req, fuse_ino_t inode, size_t size, off_t off, struct fuse_file_info* fileInfo);
    static void FuseOpen(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseRead(fuse_req_t req, fuse_ino_t inode, size_t size, off_t off, struct fuse_file_info* fileInfo);
    static void FuseWriteBuf(fuse_req_t req, fuse_ino_t ino, struct fuse_bufvec* bufv, off_t off, struct fuse_file_info* fileInfo);
    static void FuseOpendir(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseForgetmulti(fuse_req_t req, size_t count, fuse_forget_data* forgets);
    static void FuseRelease(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info* fileInfo);
    static void FuseReleasedir(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info* fileInfo);
    static void FuseCopyFileRange(fuse_req_t req, fuse_ino_t ino_in,
        off_t off_in, struct fuse_file_info* fi_in,
        fuse_ino_t ino_out, off_t off_out,
        struct fuse_file_info* fi_out, size_t len,
        int flags);

    fuse_lowlevel_ops _fuseOperations;
    FileTree _tree;
    std::string _mountpoint;
};

} // namespace FastTransport::FileSystem
