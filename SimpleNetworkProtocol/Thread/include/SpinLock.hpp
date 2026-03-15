#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

#ifdef __SANITIZE_THREAD__
#include <sanitizer/tsan_interface.h>
#endif

namespace FastTransport::Thread {

class SpinLock {
public:
    SpinLock()
        : _flag(0)
    {
    }

    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;
    ~SpinLock() = default;

    void lock()
    {
#ifdef __SANITIZE_THREAD__
        __tsan_mutex_pre_lock(&_tsanHB, 0);
#endif
        for (int i = 0; (_flag.load(std::memory_order_relaxed) != 0U) || (_flag.exchange(1, std::memory_order_acquire) != 0U); i++) {
            if (i == 10) {
                i = 0;
                std::this_thread::yield();
            }
        }
#ifdef __SANITIZE_THREAD__
        __tsan_mutex_post_lock(&_tsanHB, 0, 0);
#endif
    }

    void unlock()
    {
#ifdef __SANITIZE_THREAD__
        __tsan_mutex_pre_unlock(&_tsanHB, 0);
#endif
        _flag.store(0, std::memory_order_release);
#ifdef __SANITIZE_THREAD__
        __tsan_mutex_post_unlock(&_tsanHB, 0);
#endif
    }

private:
    std::atomic<uint32_t> _flag;
#ifdef __SANITIZE_THREAD__
    char _tsanHB = 0;
#endif
};
} // namespace FastTransport::Thread
