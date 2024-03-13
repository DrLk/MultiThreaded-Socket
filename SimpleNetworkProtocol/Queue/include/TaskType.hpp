#pragma once

namespace TaskQueue {

enum class TaskType {
    None,
    Main,
    NetworkRead,
    NetworkWrite,
    DiskRead,
    DiskWrite
};

} // namespace TaskQueue
