#include "FileSystem.hpp"

#include <functional>
#include <fuse3/fuse.h>
#include <unordered_map>

#include "FileTree.hpp"

namespace FastTransport::FileSystem {
void FileSystem::Start()
{
    fuse_args args = FUSE_ARGS_INIT(0, nullptr);
    std::string mountpoint = "/mnt/test";
    fuse_opt_add_arg(&args, mountpoint.c_str());

    fuse_session* se = fuse_session_new(&args, &_fuseOperations, sizeof(_fuseOperations), nullptr);

    if (se == nullptr)
        throw std::runtime_error("Failed to fuse_session_new");

    if (fuse_set_signal_handlers(se) != 0)
        throw std::runtime_error("Failed to fuse_set_signal_handlers");

    if (fuse_session_mount(se, mountpoint.c_str()) != 0)
        throw std::runtime_error("Failed to fuse_session_mount");

    fuse_daemonize(true);

    /* Block until ctrl+c or fusermount -u */
    int result = fuse_session_loop(se);
}

std::unordered_map<fuse_ino_t, std::reference_wrapper<Leaf>> FileSystem::_openedFiles;

fuse_ino_t FileSystem::GetINode(const Leaf& leaf)
{
    return (fuse_ino_t)(&leaf);
}

Leaf& FileSystem::GetLeaf(fuse_ino_t ino)
{
    return *((Leaf*)ino);
}
void FileSystem::Stat(fuse_ino_t ino, struct stat* stbuf, File& file)
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

    fuse_entry_param entry;

    auto& parent = parentId == FUSE_ROOT_ID ? _tree.GetRoot() : GetLeaf(parentId);
    memset(&entry, 0, sizeof(entry));
    auto file = parent.Find(name);
    if (!file) {
        fuse_reply_err(req, ENOENT);
    } else {
        (*file).get().AddRef();

        entry.ino = GetINode(*file);
        entry.attr_timeout = 1.0;
        entry.entry_timeout = 1.0;
        Stat(entry.ino, &entry.attr, (*file).get().file);

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

    struct stat stbuf;

    (void)fileInfo;

    if (inode == FUSE_ROOT_ID) {
        memset(&stbuf, 0, sizeof(stbuf));
        File file {
            .name = "test",
            .size = 0,
            .type = std::filesystem::file_type::directory,
        };
        Stat(inode, &stbuf, file);
        fuse_reply_attr(req, &stbuf, 1.0);
        return;
    }

    auto& file = GetLeaf(inode);
    memset(&stbuf, 0, sizeof(stbuf));
    Stat(inode, &stbuf, file.file);
    fuse_reply_attr(req, &stbuf, 1.0);
}

struct dirbuf {
    char* p;
    size_t size;
};

void FileSystem::BufferAddFile(fuse_req_t req, struct dirbuf* buffer, const char* name,
    fuse_ino_t ino)
{
    struct stat stbuf;
    size_t oldsize = buffer->size;
    buffer->size += fuse_add_direntry(req, nullptr, 0, name, nullptr, 0);
    buffer->p = (char*)realloc(buffer->p, buffer->size);
    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;
    fuse_add_direntry(req, buffer->p + oldsize, buffer->size - oldsize, name, &stbuf, buffer->size);
}

int ReplyBufferLimited(fuse_req_t req, const char* buffer, size_t bufferSize,
    off_t off, size_t maxSize)
{
    if (off < bufferSize)
        return fuse_reply_buf(req, buffer + off,
            std::min(bufferSize - off, maxSize));
    else
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
    auto& directory = inode == FUSE_ROOT_ID ? _tree.GetRoot() : GetLeaf(inode);
    fuse_ino_t currentINode = FUSE_ROOT_ID;
    if (inode != FUSE_ROOT_ID) {
        currentINode = GetINode(directory);
    }

    if (false)
        fuse_reply_err(req, ENOENT);
    else {
        dirbuf buffer;

        memset(&buffer, 0, sizeof(buffer));
        BufferAddFile(req, &buffer, ".", currentINode);
        BufferAddFile(req, &buffer, "..", 1);
        for (auto& [name, file] : directory.children) {
            auto inode = GetINode(file);
            BufferAddFile(req, &buffer, file.file.name.c_str(), inode);
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

    if ((fileInfo->flags & O_ACCMODE) != O_RDONLY) {
        fuse_reply_err(req, EACCES);
    }

    auto file = _openedFiles.find(inode);
    if (file == _openedFiles.end()) {
        _openedFiles.insert({ inode, GetLeaf(inode) });
        fuse_reply_open(req, fileInfo);
    } else {
        fuse_reply_open(req, fileInfo);
    }
}

void FileSystem::FuseRead(fuse_req_t req, fuse_ino_t inode, size_t size, off_t off, struct fuse_file_info* fileInfo)
{
    TRACER() << "[read]"
             << " request: " << req
             << " inode: " << inode
             << " size: " << size
             << " off: " << off;

    (void)fileInfo;

    auto file = _openedFiles.find(inode);
    if (file == _openedFiles.end())
        fuse_reply_err(req, ENOENT);
    else {
        std::vector<char> bytes;
        bytes.resize(file->second.get().file.size);
        ReplyBufferLimited(req, bytes.data(), bytes.size(), off, size);
    }
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

void FileSystem::FuseForgetmulti(fuse_req_t req, size_t count, fuse_forget_data* forgets)
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

    auto& file = GetLeaf(inode);
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

    auto& file = GetLeaf(inode);
    file.ReleaseRef();
}

FileTree FileSystem::_tree = FileTree::GetTestFileTree();

} // namespace FastTransport::FileSystem
