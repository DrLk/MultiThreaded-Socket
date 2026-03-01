#include "FileCache/SetPieceOnDiskJob.hpp"

#include "PieceStatus.hpp"
#include "PiecesStatus.hpp"

namespace FastTransport::FileCache {

SetPieceOnDiskJob::SetPieceOnDiskJob(const std::shared_ptr<FileSystem::PiecesStatus>& piecesStatus, size_t index)
    : _piecesStatus(piecesStatus)
    , _index(index)
{
}

void SetPieceOnDiskJob::ExecuteCachedTree(TaskQueue::ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& /*tree*/)
{
    _piecesStatus->SetStatus(_index, FileSystem::PieceStatus::OnDisk);
}

} // namespace FastTransport::FileCache
