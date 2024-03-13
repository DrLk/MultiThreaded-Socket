#pragma once

#include <cstddef>

namespace TaskQueue {

class Stream {
public:
    Stream() = default;
    Stream(const Stream&) = delete;
    Stream(Stream&&) = delete;
    virtual ~Stream() = default;
    Stream& operator=(const Stream&) = delete;
    Stream& operator=(Stream&&) = delete;

    virtual Stream& write(const void* data, std::size_t size) = 0;
    virtual Stream& read(void* data, std::size_t size) = 0;
    virtual Stream& flush() = 0;
};

} // namespace TaskQueue
