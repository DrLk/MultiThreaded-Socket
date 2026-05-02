#include "ProcessInotifyEventJob.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stop_token>
#include <system_error>
#include <utility>

#include "FileSystem/SendNotifyInvalEntryJob.hpp"
#include "FileSystem/SendNotifyInvalInodeJob.hpp"
#include "FileTree.hpp"
#include "ITaskScheduler.hpp"
#include "InotifyWatcher.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[ProcessInotifyEventJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

namespace {

    // Walk path components from root downwards. O(depth * log(width)) instead of
    // the O(tree size) DFS that the previous FindLeafByPath did.
    FileSystem::Leaf* ResolvePath(FileSystem::FileTree& fileTree, const std::filesystem::path& target)
    {
        const std::filesystem::path rootPath = fileTree.GetRoot().GetFullPath();
        std::error_code errCode;
        const std::filesystem::path rel = std::filesystem::relative(target, rootPath, errCode);
        if (errCode || rel.empty() || *rel.begin() == "..") {
            return nullptr;
        }
        FileSystem::Leaf* node = &fileTree.GetRoot();
        if (rel == ".") {
            return node;
        }
        for (const auto& part : rel) {
            FileSystem::Leaf* const child = node->FindChild(part.string());
            if (child == nullptr) {
                return nullptr;
            }
            node = child;
        }
        return node;
    }

} // namespace

ProcessInotifyEventJob::ProcessInotifyEventJob(FileSystem::WatchEvent event, FileSystem::FileTree& fileTree)
    : _event(std::move(event))
    , _fileTree(fileTree)
{
}

void ProcessInotifyEventJob::ExecuteMainRead(std::stop_token /*stop*/, ITaskScheduler& scheduler)
{
    FileSystem::FileTree& fileTree = _fileTree.get();
    switch (_event.type) {
    case FileSystem::WatchEventType::Modified: {
        const FileSystem::Leaf* const leaf = ResolvePath(fileTree, _event.path);
        if (leaf != nullptr) {
            TRACER() << "Modified: " << _event.path;
            scheduler.Schedule(std::make_unique<SendNotifyInvalInodeJob>(leaf->GetServerInode()));
        }
        break;
    }
    case FileSystem::WatchEventType::Created:
    case FileSystem::WatchEventType::MovedTo: {
        FileSystem::Leaf* const parentLeaf = ResolvePath(fileTree, _event.path.parent_path());
        if (parentLeaf == nullptr) {
            break;
        }
        TRACER() << "Created: " << _event.path;
        std::error_code errCode;
        const auto fileType = _event.isDir ? std::filesystem::file_type::directory : std::filesystem::file_type::regular;
        const std::uintmax_t size = _event.isDir ? 0 : std::filesystem::file_size(_event.path, errCode);
        const std::uintmax_t resolvedSize = errCode ? 0 : size;
        const FileSystem::Leaf& newLeaf = parentLeaf->AddChild(std::filesystem::path(_event.name), fileType, resolvedSize);
        const std::uint64_t newServerInode = newLeaf.GetServerInode();
        const std::uint64_t parentServerInode = parentLeaf->GetServerInode();
        scheduler.Schedule(std::make_unique<SendNotifyInvalEntryJob>(
            _event.type, parentServerInode, _event.name.string(), newServerInode, resolvedSize, _event.isDir));
        scheduler.Schedule(std::make_unique<SendNotifyInvalInodeJob>(parentServerInode));
        break;
    }
    case FileSystem::WatchEventType::Deleted:
    case FileSystem::WatchEventType::MovedFrom: {
        FileSystem::Leaf* const parentLeaf = ResolvePath(fileTree, _event.path.parent_path());
        if (parentLeaf == nullptr) {
            break;
        }
        TRACER() << "Deleted: " << _event.path;
        const std::uint64_t parentServerInode = parentLeaf->GetServerInode();
        auto nameStr = _event.name.string();
        parentLeaf->RemoveChild(nameStr);
        scheduler.Schedule(std::make_unique<SendNotifyInvalEntryJob>(
            _event.type, parentServerInode, std::move(nameStr), 0, 0, false));
        scheduler.Schedule(std::make_unique<SendNotifyInvalInodeJob>(parentServerInode));
        break;
    }
    }
}

} // namespace FastTransport::TaskQueue
