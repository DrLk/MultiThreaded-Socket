#pragma once

namespace FastTransport::TaskQueue {

enum class TaskType {
    None,
    Main,
    NetworkRead,
    NetworkWrite,
    DiskRead,
    DiskWrite
};

} // namespace FastTransport::TaskQueue
