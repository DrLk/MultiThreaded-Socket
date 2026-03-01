#pragma once

#include <cstddef>
#include <memory>
#include <sys/types.h>

#include "DiskJob.hpp"
#include "MultiList.hpp"

namespace FastTransport::FileSystem {
class NativeFile;
class PiecesStatus;
} // namespace FastTransport::FileSystem

namespace FastTransport::FileCache {

class EvictLeafBlockJob : public TaskQueue::DiskJob {
public:
    EvictLeafBlockJob(Data&& data, off_t offset, size_t size, size_t index,
        const std::shared_ptr<FileSystem::PiecesStatus>& piecesStatus, const std::shared_ptr<FileSystem::NativeFile>& file);
    Data ExecuteDisk(TaskQueue::ITaskScheduler& scheduler, Data&& free) override;

private:
    Data _data;
    off_t _offset;
    size_t _size;
    size_t _index;
    std::shared_ptr<FileSystem::PiecesStatus> _piecesStatus;
    std::shared_ptr<FileSystem::NativeFile> _file;
};

} // namespace FastTransport::FileCache
