#pragma once

#include <cstdint>
#include <sys/types.h>
#include <type_traits>
#define FUSE_USE_VERSION 34

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <fuse3/fuse_lowlevel.h>
#include <unordered_map>

#include "File.hpp"
#include "FileTree.hpp"
#include "Logger.hpp"

namespace FastTransport::FileSystem {
class FileSystem final {
    using FilePtr = std::unique_ptr<File>;

public:
    FileSystem(std::string_view mountpoint);
    void Start();

private:
    static std::unordered_map<fuse_ino_t, std::reference_wrapper<Leaf>> _openedFiles;

    struct dirbuf {
        char* p;
        size_t size;
    } __attribute__((aligned(16)));

    static void BufferAddFile(fuse_req_t req, struct dirbuf* buffer, const char* name, fuse_ino_t ino);
    static int ReplyBufferLimited(fuse_req_t req, const char* buffer, size_t bufferSize, off_t off, size_t maxSize);
    static void Stat(fuse_ino_t ino, struct stat* stbuf, const File& file);

    static void FuseLookup(fuse_req_t req, fuse_ino_t parentId, const char* name);
    static void FuseForget(fuse_req_t req, fuse_ino_t inode, uint64_t nlookup);
    static void FuseGetattr(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseReaddir(fuse_req_t req, fuse_ino_t inode, size_t size, off_t off, struct fuse_file_info* fileInfo);
    static void FuseOpen(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseRead(fuse_req_t req, fuse_ino_t inode, size_t size, off_t off, struct fuse_file_info* fileInfo);
    static void FuseOpendir(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseForgetmulti(fuse_req_t req, size_t count, fuse_forget_data* forgets);
    static void FuseRelease(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info* fileInfo);
    static void FuseReleasedir(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info* fileInfo);

    fuse_lowlevel_ops _fuseOperations;
    FileTree _tree;
    std::string _mountpoint;
};

} // namespace FastTransport::FileSystem
