#include "FileSystem.hpp"

#include <cstring>
#include <errno.h>
#include <fuse3/fuse.h>

namespace FastTransport::Protocol {

static struct options {
    const char* filename;
    const char* contents;
    int show_help;
} options;

void* FileSystem::FuseInit(struct fuse_conn_info* conn, struct fuse_config* cfg)
{
	options.filename = strdup("hello");
	options.contents = strdup("Hello World!\n");

    (void)conn;
    cfg->kernel_cache = 1;
    return nullptr;
}

int FileSystem::FuseGetAttr(const char* path, struct stat* stbuf,
    struct fuse_file_info* fi)
{
    (void)fi;
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path + 1, options.filename) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(options.contents);
    } else

        res = -ENOENT;

    return res;
}

int FileSystem::FuseReadDir(const char* path, void* buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info* fi,
    enum fuse_readdir_flags flags)
{
    (void)offset;
    (void)fi;
    (void)flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;
    fuse_fill_dir_flags fillDirFlags { (fuse_fill_dir_flags)0 };
    filler(buf, ".", NULL, 0, fillDirFlags);
    filler(buf, "..", NULL, 0, fillDirFlags);
    filler(buf, options.filename, NULL, 0, fillDirFlags);

    return 0;
}

} // namespace FastTransport::Protocol
