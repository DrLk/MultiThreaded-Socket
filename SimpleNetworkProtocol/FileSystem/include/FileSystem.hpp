#pragma once

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

namespace FastTransport::Protocol {
class FileSystem {

private:
    static void* FuseInit(struct fuse_conn_info* conn, struct fuse_config* cfg);
    static int FuseGetAttr(const char* path, struct stat* stbuf, struct fuse_file_info* fi);
    static int FuseReadDir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
};

} // namespace FastTransport::Protocol
