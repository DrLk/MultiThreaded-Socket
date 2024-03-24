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
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

#include "File.hpp"
#include "FileTree.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileSystem {

namespace {
    fuse_ino_t GetINode(const Leaf& leaf)
    {
        return (fuse_ino_t)(&leaf);
    }

    Leaf& GetLeaf(fuse_ino_t ino)
    {
        return *((Leaf*)ino);
    }
}

FileSystem::FileSystem(std::string_view mountpoint)
    : _fuseOperations {
        .lookup = FuseLookup,
        .forget = FuseForget,
        .getattr = FuseGetattr,
        .open = FuseOpen,
        .read = FuseRead,
        .release = FuseRelease,
        .opendir = FuseOpendir,
        .readdir = FuseReaddir,
        .releasedir = FuseReleasedir,
        .forget_multi = FuseForgetmulti,
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

    fuse_entry_param entry {};

    FileTree* tree = nullptr;
    if (parentId == FUSE_ROOT_ID) {
        tree = static_cast<FileTree*>(fuse_req_userdata(req));
    }
    const auto& parent = FUSE_ROOT_ID == parentId ? tree->GetRoot() : GetLeaf(parentId);
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

    auto& file = GetLeaf(inode);
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

    if (inode == FUSE_ROOT_ID) {
        FileTree* tree = static_cast<FileTree*>(fuse_req_userdata(req));
        memset(&stbuf, 0, sizeof(stbuf));
        Stat(inode, &stbuf, tree->GetRoot().GetFile());
        fuse_reply_attr(req, &stbuf, 1.0);
        return;
    }

    auto& file = GetLeaf(inode);
    memset(&stbuf, 0, sizeof(stbuf));
    Stat(inode, &stbuf, file.GetFile());
    fuse_reply_attr(req, &stbuf, 1.0);
}

struct dirbuf {
    char* p;
    size_t size;
} __attribute__((aligned(16)));

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

    auto& directory = inode == FUSE_ROOT_ID ? tree->GetRoot() : GetLeaf(inode);
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


    const auto& leaf = GetLeaf(inode);
    leaf.AddRef();

    std::filesystem::path root = "/tmp";
    std::filesystem::path path = root / leaf.GetFullPath();

    if ((fileInfo->flags & O_ACCMODE) != O_RDONLY) {
        fuse_reply_err(req, EACCES);
    }


	int fd = open(path.c_str(), fileInfo->flags & ~O_NOFOLLOW);
	if (fd == -1)
    {
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

	fuse_reply_data(req, &buf, FUSE_BUF_SPLICE_MOVE);

    /*
    auto file = _openedFiles.find(inode);
    if (file == _openedFiles.end()) {
        fuse_reply_err(req, ENOENT);
    } else {
        std::vector<char> bytes;
        auto writedSize = GetLeaf(inode).Read(off, std::as_writable_bytes(std::span(bytes)));
        bytes.resize(file->second.get().GetFile().size);
        ReplyBufferLimited(req, bytes.data(), bytes.size(), off, writedSize);
    }*/
}

void FileSystem::FuseOpendir(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    TRACER() << "[opendir]"
             << " request: " << req
             << " inode: " << inode;

    if ((fileInfo->flags & O_ACCMODE) != O_RDONLY) {
        fuse_reply_err(req, EACCES);
    }

    if (inode == FUSE_ROOT_ID) {
        fuse_reply_open(req, fileInfo);
        return;
    }

    auto& file = GetLeaf(inode);
    file.AddRef();

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

    auto& file = GetLeaf(inode);
    file.ReleaseRef();
}

void FileSystem::FuseReleasedir(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info* /*fileInfo*/)
{
    TRACER() << "[releasedir]"
             << " request: " << req
             << " inode: " << inode;

    if (inode == FUSE_ROOT_ID) {
        return;
    }

    auto& file = GetLeaf(inode);
    file.ReleaseRef();
}

} // namespace FastTransport::FileSystem
