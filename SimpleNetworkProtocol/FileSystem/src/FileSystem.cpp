#include "FileSystem.hpp"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <fuse3/fuse_common.h>
#include <fuse3/fuse_lowlevel.h>
#include <fuse3/fuse_opt.h>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "File.hpp"
#include "FileTree.hpp"
#include "FuseRequestTracker.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[FileSystem] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileSystem {

namespace {
    fuse_ino_t GetINode(const Leaf& leaf)
    {
        return reinterpret_cast<fuse_ino_t>(&leaf); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }

    Leaf& GetLeaf(fuse_ino_t inode, fuse_req_t req)
    {
        FileTree* tree = nullptr;
        if (inode == FUSE_ROOT_ID) {
            tree = static_cast<FileTree*>(fuse_req_userdata(req));
        }

        return inode == FUSE_ROOT_ID ? tree->GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    }
} // namespace

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

void FileSystem::Init()
{
    if (_session != nullptr) {
        return;
    }

    _fuseOperations = {
        .init = FuseInit,
        .lookup = *_lookup.target<void (*)(fuse_req_t, fuse_ino_t, const char*)>(),
        .forget = *_forget.target<void (*)(fuse_req_t, fuse_ino_t, uint64_t)>(),
        .getattr = *_getattr.target<void (*)(fuse_req_t, fuse_ino_t, fuse_file_info*)>(),
        .open = *_open.target<void (*)(fuse_req_t, fuse_ino_t, fuse_file_info*)>(),
        .read = *_read.target<void (*)(fuse_req_t, fuse_ino_t, size_t, off_t, fuse_file_info*)>(),
        .release = *_release.target<void (*)(fuse_req_t, fuse_ino_t, fuse_file_info*)>(),
        .opendir = *_opendir.target<void (*)(fuse_req_t, fuse_ino_t, fuse_file_info*)>(),
        /* .readdir = *_readDir.target<void (*)(fuse_req_t, fuse_ino_t, size_t, off_t, fuse_file_info*)>(), */
        .releasedir = *_releaseDir.target<void (*)(fuse_req_t, fuse_ino_t, fuse_file_info*)>(),
        .write_buf = FuseWriteBuf,
        .forget_multi = *_forgetMulti.target<void (*)(fuse_req_t, size_t, fuse_forget_data*)>(),
        .readdirplus = *_readDirPlus.target<void (*)(fuse_req_t, fuse_ino_t, size_t, off_t, fuse_file_info*)>(),
        .copy_file_range = FuseCopyFileRange,
    };

    fuse_args args = FUSE_ARGS_INIT(0, nullptr);
    fuse_opt_add_arg(&args, _mountpoint.c_str());
    fuse_opt_add_arg(&args, "-oauto_unmount");

    _session = fuse_session_new(&args, &_fuseOperations, sizeof(_fuseOperations), &_tree);
    fuse_opt_free_args(&args);

    if (_session == nullptr) {
        throw std::runtime_error("Failed to fuse_session_new");
    }

    if (fuse_set_signal_handlers(_session) != 0) {
        fuse_session_destroy(_session);
        _session = nullptr;
        throw std::runtime_error("Failed to fuse_set_signal_handlers");
    }

    if (fuse_session_mount(_session, _mountpoint.c_str()) != 0) {
        fuse_remove_signal_handlers(_session);
        fuse_session_destroy(_session);
        _session = nullptr;
        throw std::runtime_error("Failed to fuse_session_mount");
    }
}

void FileSystem::Start(std::stop_token stop)
{
    Init();

    // FUSE is mounted — start multi-threaded event loop in background thread and return.
    _fuseThread = std::jthread([this, stop = std::move(stop)](std::stop_token threadStop) {
        fuse_daemonize(1);

        const std::stop_callback onStop(stop, [this]() {
            LOGGER() << "[FileSystem] Stop requested, exiting FUSE session";
            FUSE_DUMP_PENDING();
            fuse_session_exit(_session);
        });
        const std::stop_callback onThreadStop(threadStop, [this]() { fuse_session_exit(_session); });

        struct fuse_loop_config* config = fuse_loop_cfg_create();
        fuse_loop_cfg_set_clone_fd(config, 1);

        const int ret = fuse_session_loop_mt(_session, config);
        LOGGER() << "[FileSystem] fuse_session_loop_mt exited with return code: " << ret;
        FUSE_DUMP_PENDING();
        if (ret != 0) {
            LOGGER() << "[FileSystem] fuse_session_loop_mt exited with error: " << ret;
        }

        fuse_loop_cfg_destroy(config);
        fuse_remove_signal_handlers(_session);
    });
}

FileSystem::~FileSystem()
{
    if (_session != nullptr) {
        fuse_session_destroy(_session);
    }
}

void FileSystem::RequestStopAndWait()
{
    // request_stop() triggers the onThreadStop callback → fuse_session_exit(_session)
    // → fuse_session_loop returns → the fuse thread runs its cleanup and exits.
    _fuseThread.request_stop();
    if (_fuseThread.joinable()) {
        _fuseThread.join();
    }
    // After join(), ~jthread() will see !joinable() and skip the second join/request_stop.
}

// NOLINTBEGIN

void FileSystem::Stat(fuse_ino_t ino, struct stat* stbuf, const Leaf& file)
{
    stbuf->st_ino = ino;
    switch (file.GetType()) {
    case std::filesystem::file_type::directory:
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        break;

    case std::filesystem::file_type::regular:
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = file.GetSize();
        break;

    default:
        throw std::runtime_error("Not implemented");
    }
}

void FileSystem::FuseLookup(fuse_req_t req, fuse_ino_t parentId, const char* name)
{
    FUSE_TRACK(req, "lookup");
    TRACER() << "[lookup]"
             << " request: " << req
             << " parent: " << parentId
             << " name: " << name;

    const auto& parent = GetLeaf(parentId, req);
    fuse_entry_param entry {};
    memset(&entry, 0, sizeof(entry));
    auto file = parent.Find(name);
    if (!file || file.value().get().IsDeleted()) {
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(req), ENOENT));
    } else {
        (*file).get().AddRef();

        entry.ino = GetINode(*file);
        entry.attr_timeout = 1.0;
        entry.entry_timeout = 1.0;
        Stat(entry.ino, &entry.attr, file.value());

        FUSE_ASSERT_REPLY(fuse_reply_entry(FUSE_UNTRACK(req), &entry));
    }
}

void FileSystem::FuseForget(fuse_req_t req, fuse_ino_t inode, uint64_t nlookup)
{
    FUSE_TRACK(req, "forget");
    TRACER() << "[forget]"
             << " request: " << req
             << " inode: " << inode
             << " nlookup: " << nlookup;

    auto& file = GetLeaf(inode, req);
    file.ReleaseRef(nlookup);
    fuse_reply_none(FUSE_UNTRACK(req));
}

void FileSystem::FuseGetattr(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    FUSE_TRACK(req, "getattr");
    TRACER() << "[getattr]"
             << " request: " << req
             << " inode: " << inode;

    struct stat stbuf {};

    (void)fileInfo;

    auto& file = GetLeaf(inode, req);
    memset(&stbuf, 0, sizeof(stbuf));
    Stat(inode, &stbuf, file);
    FUSE_ASSERT_REPLY(fuse_reply_attr(FUSE_UNTRACK(req), &stbuf, 1.0));
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

    if ((conn->capable & FUSE_CAP_SPLICE_READ) != 0U) {
        conn->want |= FUSE_CAP_SPLICE_READ;
    }

    if ((conn->capable & FUSE_CAP_SPLICE_WRITE) != 0U) {
        conn->want |= FUSE_CAP_SPLICE_WRITE;
    }

    if ((conn->capable & FUSE_CAP_SPLICE_MOVE) != 0U) {
        conn->want |= FUSE_CAP_SPLICE_MOVE;
    }

    if ((conn->capable & FUSE_CAP_SPLICE_MOVE) != 0U) {
        conn->want |= FUSE_CAP_SPLICE_MOVE;
    }

    if ((conn->capable & FUSE_CAP_ASYNC_READ) != 0U) {
        conn->want |= FUSE_CAP_ASYNC_READ;
    }

    conn->max_readahead = 4 * 1024 * 1024;
}

void FileSystem::BufferAddFile(fuse_req_t req, struct dirbuf* buffer, const char* name,
    fuse_ino_t ino)
{
    struct stat stbuf {};
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
        return FUSE_ASSERT_REPLY(fuse_reply_buf(FUSE_UNTRACK(req), buffer + off,
            std::min(bufferSize - off, maxSize)));
    }
    return FUSE_ASSERT_REPLY(fuse_reply_buf(FUSE_UNTRACK(req), nullptr, 0));
}

void FileSystem::FuseReaddir(fuse_req_t req, fuse_ino_t inode, size_t size, off_t off, struct fuse_file_info* fileInfo)
{
    FUSE_TRACK(req, "readdir");
    TRACER() << "[readdir]"
             << " request: " << req
             << " inode: " << inode
             << " size: " << size
             << " off: " << off;

    (void)fileInfo;

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
        for (const auto& [name, file] : directory.GetChildren()) {
            auto inode = GetINode(*file);
            BufferAddFile(req, &buffer, file->GetName().c_str(), inode);
        }

        ReplyBufferLimited(req, buffer.p, buffer.size, off, size);
        free(buffer.p);
    }
}

void FileSystem::FuseOpen(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    FUSE_TRACK(req, "open");
    TRACER() << "[open]"
             << " request: " << req
             << " inode: " << inode;

    const auto& leaf = GetLeaf(inode, req);
    leaf.AddRef();

    const std::filesystem::path root = "/tmp";
    std::filesystem::path path = root / leaf.GetFullPath();

    /*if ((fileInfo->flags & O_ACCMODE) != O_RDONLY) {
        fuse_reply_err(req, EACCES);
    }*/

    const int file = open(path.c_str(), fileInfo->flags & ~O_NOFOLLOW); // NOLINT(hicpp-signed-bitwise, hicpp-vararg, cppcoreguidelines-pro-type-vararg)
    if (file == -1) {
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(req), errno));
        return;
    }

    fileInfo->fh = file;
    /* fi->direct_io = 1; */
    /* fi->keep_cache = 1; */

    if ((fileInfo->flags & O_DIRECT) != 0)
        fileInfo->direct_io = 1;

    FUSE_ASSERT_REPLY(fuse_reply_open(FUSE_UNTRACK(req), fileInfo));
}

static fuse_buf_flags operator|(fuse_buf_flags flag1, fuse_buf_flags flag2) // NOLINT(fuchsia-overloaded-operator)
{
    return static_cast<fuse_buf_flags>(static_cast<std::uint32_t>(flag1) | static_cast<std::uint32_t>(flag2));
}

void FileSystem::FuseRead(fuse_req_t req, fuse_ino_t inode, size_t size, off_t off, struct fuse_file_info* fileInfo)
{
    FUSE_TRACK(req, "read");
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

    FUSE_ASSERT_REPLY(fuse_reply_data(FUSE_UNTRACK(req), &buf, FUSE_BUF_FORCE_SPLICE));
}

void FileSystem::FuseWriteBuf(fuse_req_t req, fuse_ino_t ino, struct fuse_bufvec* bufv, off_t off, struct fuse_file_info* fileInfo)
{
    FUSE_TRACK(req, "write_buf");
    TRACER() << "[write_buf]"
             << " request: " << req
             << " inode: " << ino
             << " bufv: " << bufv
             << " off: " << off;

    (void)fileInfo;
    fuse_bufvec destination = FUSE_BUFVEC_INIT(bufv->buf[0].size);
    destination.buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
    destination.buf[0].fd = fileInfo->fh;

    ssize_t written = fuse_buf_copy(&destination, bufv, FUSE_BUF_SPLICE_NONBLOCK);
    if (written < 0) {
        assert(errno == -written);
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(req), errno));
        return;
    }

    FUSE_ASSERT_REPLY(fuse_reply_write(FUSE_UNTRACK(req), written));
}

void FileSystem::FuseOpendir(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    FUSE_TRACK(req, "opendir");
    TRACER() << "[opendir]"
             << " request: " << req
             << " inode: " << inode;

    auto& leaf = GetLeaf(inode, req);
    auto path = leaf.GetFullPath();
    leaf.AddRef();

    const int file = open(path.c_str(), fileInfo->flags & ~O_NOFOLLOW); // NOLINT(hicpp-signed-bitwise, hicpp-vararg, cppcoreguidelines-pro-type-vararg)
    if (file == -1) {
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(req), errno));
        return;
    }

    fileInfo->fh = file;

    FUSE_ASSERT_REPLY(fuse_reply_open(FUSE_UNTRACK(req), fileInfo));
}

void FileSystem::FuseForgetmulti(fuse_req_t req, size_t count, fuse_forget_data* /*forgets*/)
{
    FUSE_TRACK(req, "forgetmulti");
    TRACER() << "[forgetmulti]"
             << " request: " << req
             << " size: " << count;
    fuse_reply_none(FUSE_UNTRACK(req));
}

void FileSystem::FuseRelease(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info* fileInfo)
{
    FUSE_TRACK(req, "release");
    TRACER() << "[release]"
             << " request: " << req
             << " inode: " << inode;

    const int error = close(fileInfo->fh);
    if (error != 0) {
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(req), errno));
        return;
    }

    auto& file = GetLeaf(inode, req);
    file.ReleaseRef();
}

void FileSystem::FuseReleasedir(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info* fileInfo)
{
    FUSE_TRACK(req, "releasedir");
    TRACER() << "[releasedir]"
             << " request: " << req
             << " inode: " << inode;

    if (inode == FUSE_ROOT_ID) {
        return;
    }

    const int error = close(fileInfo->fh);
    if (error != 0) {
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(req), errno));
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
    FUSE_TRACK(req, "copy_file_range");
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

    int fileIn = fi_in->fh;
    int fileOut = fi_out->fh;

    const ssize_t size = copy_file_range(fileIn, &off_in, fileOut, &off_out, len, flags);
    if (size < 0) {
        assert(errno == -size);
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(req), errno));
        return;
    }

    FUSE_ASSERT_REPLY(fuse_reply_write(FUSE_UNTRACK(req), size));
}

// NOLINTEND
} // namespace FastTransport::FileSystem
