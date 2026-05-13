#pragma once

#include <cstddef>

#include <memory>

#include "CacheTreeJob.hpp"

namespace FastTransport::FileSystem {
class PiecesStatus;
} // namespace FastTransport::FileSystem

namespace FastTransport::FileCache {

class SetPieceOnDiskJob : public TaskQueue::CacheTreeJob {
public:
    SetPieceOnDiskJob(const std::shared_ptr<FileSystem::PiecesStatus>& piecesStatus, size_t index);
    void ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree) override;

private:
    std::shared_ptr<FileSystem::PiecesStatus> _piecesStatus;
    size_t _index;
};

} // namespace FastTransport::FileCache
