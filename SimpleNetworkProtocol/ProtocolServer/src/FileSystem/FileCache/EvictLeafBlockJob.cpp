#include "FileCache/EvictLeafBlockJob.hpp"
#include <Tracy.hpp>

#include <memory>

#include "FileCache/SetPieceOnDiskJob.hpp"
#include "ITaskScheduler.hpp"
#include "NativeFile.hpp"

namespace FastTransport::FileCache {

EvictLeafBlockJob::EvictLeafBlockJob(Data&& data, off_t offset, size_t size, size_t index,
    const std::shared_ptr<FileSystem::PiecesStatus>& piecesStatus, const std::shared_ptr<FileSystem::NativeFile>& file)
    : _data(std::move(data))
    , _offset(offset)
    , _size(size)
    , _index(index)
    , _piecesStatus(piecesStatus)
    , _file(file)
{
}

TaskQueue::DiskJob::Data EvictLeafBlockJob::ExecuteDisk(TaskQueue::ITaskScheduler& scheduler, Data&& free)
{
    ZoneScopedN("EvictLeafBlockJob::ExecuteDisk");
    if (_data.empty()) {
        return std::move(free);
    }

    _file->Write(_data, _size, _offset);
    scheduler.Schedule(std::make_unique<SetPieceOnDiskJob>(_piecesStatus, _index));

    free.splice(std::move(_data));
    return std::move(free);
}

} // namespace FastTransport::FileCache
