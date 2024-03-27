#include "FileSystem.hpp"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <fuse3/fuse_lowlevel.h>
#include <fuse3/fuse_opt.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>

#include "File.hpp"
#include "FileTree.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"

#define TRACER() if ( 1 > 0) LOGGER() // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileSystem {

namespace {
    fuse_ino_t GetINode(const Leaf& leaf)
    {
        return (fuse_ino_t)(&leaf);
    }

    Leaf& GetLeaf(fuse_ino_t inode, fuse_req_t req)
    {
        FileTree* tree = nullptr;
        if (inode == FUSE_ROOT_ID) {
            tree = static_cast<FileTree*>(fuse_req_userdata(req));
        }

        return inode == FUSE_ROOT_ID ? tree->GetRoot() : *((Leaf*)inode);
    }
}

FileSystem::FileSystem(std::string_view mountpoint)
    : _fuseOperations {
        .init = FuseInit,
        .lookup = FuseLookup,
        .forget = FuseForget,
        .getattr = FuseGetattr,
        .open = FuseOpen,
        .read = FuseRead,
        .release = FuseRelease,
        .opendir = FuseOpendir,
        .readdir = FuseReaddir,
        .releasedir = FuseReleasedir,
        .write_buf = FuseWriteBuf,
        .forget_multi = FuseForgetmulti,
        .copy_file_range = FuseCopyFileRange,
        /*.setxattr = FuseSetxattr,
        .getxattr = FuseGetxattr,
        .removexattr = FuseRemovexattr,*/
    }
    , _tree(FileTree::GetTestFileTree())
    , _mountpoint(mountpoint)

{
}

void FileSystem::Start()
{
    fuse_args args = FUSE_ARGS_INIT(0, nullptr);
    fuse_opt_add_arg(&args, _mountpoint.c_str());
    fuse_opt_add_arg(&args, "-oauto_unmount");

    fuse_session* se = fuse_session_new(&args, &_fuseOperations, sizeof(_fuseOperations), &_tree);

    if (se == nullptr) {
        throw std::runtime_error("Failed to fuse_session_new");
    }

    if (fuse_set_signal_handlers(se) != 0) {
        throw std::runtime_error("Failed to fuse_set_signal_handlers");
    }

    if (fuse_session_mount(se, _mountpoint.c_str()) != 0) {
        throw std::runtime_error("Failed to fuse_session_mount");
    }

    fuse_daemonize(1);

    /* Block until ctrl+c or fusermount -u */
    const int result = fuse_session_loop(se);
}

std::unordered_map<fuse_ino_t, std::reference_wrapper<Leaf>> FileSystem::_openedFiles;
void FileSystem::Stat(fuse_ino_t ino, struct stat* stbuf, const File& file)
{
    stbuf->st_ino = ino;
    switch (file.type) {
    case std::filesystem::file_type::directory:
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        break;

    case std::filesystem::file_type::regular:
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = file.size;
        break;

    default:
        throw std::runtime_error("Not implemented");
    }
}

void FileSystem::FuseLookup(fuse_req_t req, fuse_ino_t parentId, const char* name)
{
    TRACER() << "[lookup]"
             << " request: " << req
             << " parent: " << parentId
             << " name: " << name;


    const auto& parent = GetLeaf(parentId, req);
    fuse_entry_param entry {};
    memset(&entry, 0, sizeof(entry));
    auto file = parent.Find(name);
    if (!file) {
        fuse_reply_err(req, ENOENT);
    } else {
        (*file).get().AddRef();

        entry.ino = GetINode(*file);
        entry.attr_timeout = 1.0;
        entry.entry_timeout = 1.0;
        Stat(entry.ino, &entry.attr, (*file).get().GetFile());

        fuse_reply_entry(req, &entry);
    }
}

void FileSystem::FuseForget(fuse_req_t req, fuse_ino_t inode, uint64_t nlookup)
{
    TRACER() << "[forget]"
             << " request: " << req
             << " inode: " << inode
             << " nlookup: " << nlookup;

    auto& file = GetLeaf(inode, req);
    file.ReleaseRef(nlookup);
    fuse_reply_none(req);
}

void FileSystem::FuseGetattr(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    TRACER() << "[getattr]"
             << " request: " << req
             << " inode: " << inode;

    struct stat stbuf { };

    (void)fileInfo;

    auto& file = GetLeaf(inode, req);
    memset(&stbuf, 0, sizeof(stbuf));
    Stat(inode, &stbuf, file.GetFile());
    fuse_reply_attr(req, &stbuf, 1.0);
}

struct dirbuf {
    char* p;
    size_t size;
} __attribute__((aligned(16)));

void FileSystem::FuseInit(void* userdata, struct fuse_conn_info* conn)
{
    TRACER() << "[init]"
             << " userdata: " << userdata
             << " conn: " << conn;

    if (conn->capable & FUSE_CAP_SPLICE_READ) {
        conn->want |= FUSE_CAP_SPLICE_READ;
    }

    if (conn->capable & FUSE_CAP_SPLICE_WRITE) {
        conn->want |= FUSE_CAP_SPLICE_WRITE;
    }

    if (conn->capable & FUSE_CAP_SPLICE_MOVE) {
        conn->want |= FUSE_CAP_SPLICE_MOVE;
    }

    if (conn->capable & FUSE_CAP_SPLICE_MOVE) {
        conn->want |= FUSE_CAP_SPLICE_MOVE;
    }
}

void FileSystem::BufferAddFile(fuse_req_t req, struct dirbuf* buffer, const char* name,
    fuse_ino_t ino)
{
    struct stat stbuf { };
    const size_t oldsize = buffer->size;
    buffer->size += fuse_add_direntry(req, nullptr, 0, name, nullptr, 0);
    buffer->p = static_cast<char*>(realloc(buffer->p, buffer->size));
    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;
    fuse_add_direntry(req, buffer->p + oldsize, buffer->size - oldsize, name, &stbuf, buffer->size);
}

int FileSystem::ReplyBufferLimited(fuse_req_t req, const char* buffer, size_t bufferSize,
    off_t off, size_t maxSize)
{
    if (off < bufferSize) {
        return fuse_reply_buf(req, buffer + off,
            std::min(bufferSize - off, maxSize));
    }
    return fuse_reply_buf(req, nullptr, 0);
}

void FileSystem::FuseReaddir(fuse_req_t req, fuse_ino_t inode, size_t size, off_t off, struct fuse_file_info* fileInfo)
{
    TRACER() << "[readdir]"
             << " request: " << req
             << " inode: " << inode
             << " size: " << size
             << " off: " << off;

    (void)fileInfo;
    FileTree* tree = nullptr;
    if (inode == FUSE_ROOT_ID) {
        tree = static_cast<FileTree*>(fuse_req_userdata(req));
    }

    auto& directory = GetLeaf(inode, req);
    fuse_ino_t currentINode = FUSE_ROOT_ID;
    if (inode != FUSE_ROOT_ID) {
        currentINode = GetINode(directory);
    }

    {
        dirbuf buffer {};

        memset(&buffer, 0, sizeof(buffer));
        BufferAddFile(req, &buffer, ".", currentINode);
        BufferAddFile(req, &buffer, "..", 1);
        for (auto& [name, file] : directory.children) {
            auto inode = GetINode(file);
            BufferAddFile(req, &buffer, file.GetFile().GetName().c_str(), inode);
            _openedFiles.insert({ inode, file });
        }

        ReplyBufferLimited(req, buffer.p, buffer.size, off, size);
        free(buffer.p);
    }
}

void FileSystem::FuseOpen(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    TRACER() << "[open]"
             << " request: " << req
             << " inode: " << inode;


    const auto& leaf = GetLeaf(inode, req);
    leaf.AddRef();

    std::filesystem::path root = "/tmp";
    std::filesystem::path path = root / leaf.GetFullPath();

    /*if ((fileInfo->flags & O_ACCMODE) != O_RDONLY) {
        fuse_reply_err(req, EACCES);
    }*/

    int fd = open(path.c_str(), fileInfo->flags & ~O_NOFOLLOW);
    if (fd == -1) {
        fuse_reply_err(req, errno);
        return;
    }

    fileInfo->fh = fd;
    /* fi->direct_io = 1; */
    /* fi->keep_cache = 1; */

	if (fileInfo->flags & O_DIRECT)
		fileInfo->direct_io = 1;

	fuse_reply_open(req, fileInfo);
}

static fuse_buf_flags operator|(fuse_buf_flags a, fuse_buf_flags b)
{
    return static_cast<fuse_buf_flags>(static_cast<int>(a) | static_cast<int>(b));
}

void FileSystem::FuseRead(fuse_req_t req, fuse_ino_t inode, size_t size, off_t off, struct fuse_file_info* fileInfo)
{
    TRACER() << "[read]"
             << " request: " << req
             << " inode: " << inode
             << " size: " << size
             << " off: " << off;

    (void)fileInfo;

    struct fuse_bufvec buf = FUSE_BUFVEC_INIT(size);

    buf.buf[0].flags = fuse_buf_flags::FUSE_BUF_IS_FD | fuse_buf_flags::FUSE_BUF_FD_SEEK;
	buf.buf[0].fd = fileInfo->fh;
	buf.buf[0].pos = off;

	fuse_reply_data(req, &buf, FUSE_BUF_FORCE_SPLICE);
}

void FileSystem::FuseWriteBuf(fuse_req_t req, fuse_ino_t ino, struct fuse_bufvec* bufv, off_t off, struct fuse_file_info* fi)
{
    TRACER() << "[write_buf]"
             << " request: " << req
             << " inode: " << ino
             << " bufv: " << bufv
             << " off: " << off;

    (void)fi;
    fuse_bufvec destination = FUSE_BUFVEC_INIT(bufv->buf[0].size);
    destination.buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
    destination.buf[0].fd = fi->fh;

    ssize_t written = fuse_buf_copy(&destination, bufv, FUSE_BUF_SPLICE_NONBLOCK);
    if (written < 0) {
        fuse_reply_err(req, errno);
        return;
    }

    fuse_reply_write(req, written);
}

void FileSystem::FuseOpendir(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    TRACER() << "[opendir]"
             << " request: " << req
             << " inode: " << inode;

    auto& leaf = GetLeaf(inode, req);
    auto path = leaf.GetFullPath();
    leaf.AddRef();

    int fd = open(path.c_str(), fileInfo->flags & ~O_NOFOLLOW);
    if (fd == -1) {
        fuse_reply_err(req, errno);
        return;
    }

    fileInfo->fh = fd;

    if (inode == FUSE_ROOT_ID) {
        fuse_reply_open(req, fileInfo);
        return;
    }

    fuse_reply_open(req, fileInfo);
}

void FileSystem::FuseForgetmulti(fuse_req_t req, size_t count, fuse_forget_data* /*forgets*/)
{
    TRACER() << "[forgetmulti]"
             << " request: " << req
             << " size: " << count;
    fuse_reply_none(req);
}

void FileSystem::FuseRelease(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info* fileInfo)
{
    TRACER() << "[release]"
             << " request: " << req
             << " inode: " << inode;

    int error = close(fileInfo->fh);
    if (error != 0) {
        fuse_reply_err(req, errno);
        return;
    }

    auto& file = GetLeaf(inode, req);
    file.ReleaseRef();
}

void FileSystem::FuseReleasedir(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info* fileInfo)
{
    TRACER() << "[releasedir]"
             << " request: " << req
             << " inode: " << inode;

    if (inode == FUSE_ROOT_ID) {
        return;
    }

    int error = close(fileInfo->fh);
    if (error != 0) {
        fuse_reply_err(req, errno);
        return;
    }

    auto& file = GetLeaf(inode, req);
    file.ReleaseRef();
}

void FileSystem::FuseCopyFileRange(fuse_req_t req, fuse_ino_t ino_in,
    off_t off_in, struct fuse_file_info* fi_in,
    fuse_ino_t ino_out, off_t off_out,
    struct fuse_file_info* fi_out, size_t len,
    int flags)
{
    TRACER() << "[copy_file_range]"
             << " request: " << req
             << " ino_in: " << ino_in
             << " off_in: " << off_in
             << " fi_in: " << fi_in
             << " ino_out: " << ino_out
             << " off_out: " << off_out
             << " fi_out: " << fi_out
             << " len: " << len
             << " flags: " << flags;
    int i = 0;
    i++;
    return;
}

} // namespace FastTransport::FileSystem
