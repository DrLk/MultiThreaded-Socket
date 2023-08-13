#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

namespace FastTransport::Protocol {

class SpinLock {
public:
    SpinLock()
        : _flag(0)
    {
    }

    void lock()
    {
        for (int i = 0; (_flag.load(std::memory_order_relaxed) != 0U) || (_flag.exchange(1, std::memory_order_acquire) != 0U); i++) {
            if (i == 10) {
                i = 0;
                std::this_thread::yield();
            }
        }
    }

    void unlock()
    {
        _flag.store(0, std::memory_order_release);
    }

private:
    std::atomic<uint32_t> _flag;
};
} // namespace FastTransport::Protocol