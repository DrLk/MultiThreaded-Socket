#pragma once

#ifdef __linux__

#include <atomic>
#include <optional>
#include <sys/mman.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>
#include <utility>

// RAII wrapper around a child process, modelled after std::jthread.
//
// Why child processes instead of threads: a debugger breakpoint halts every
// thread in the process, including the FUSE session thread. Any thread inside
// the same process that is blocked waiting for a FUSE reply will never wake up,
// causing a deadlock. Child processes are independent OS schedulable entities;
// a breakpoint in the parent only halts the parent, so the child can still
// send its FUSE request and receive the reply once the parent is resumed.
//
// The callable may optionally accept a jprocess::stop_token as its first
// argument (detected at compile time via a requires-expression). The stop_token
// is backed by an anonymous shared-memory page so that request_stop() in the
// parent is immediately visible to the child.
class jprocess {
public:
    // stop_token backed by a shared-memory std::atomic<bool>.
    class stop_token {
        friend class jprocess;
        std::atomic<bool>* flag_;
        explicit stop_token(std::atomic<bool>* flag) noexcept
            : flag_(flag)
        {
        }

    public:
        [[nodiscard]] bool stop_requested() const noexcept
        {
            return flag_->load(std::memory_order_relaxed);
        }
    };

    // Forks immediately; the child calls func(stop_token) (or func() if the
    // callable does not accept a stop_token) and exits with 0 on success / 1 on failure.
    // The requires-clause prevents this constructor from shadowing the move constructor.
    template <typename Func>
        requires(!std::is_same_v<std::remove_cvref_t<Func>, jprocess>)
    explicit jprocess(Func&& func)
    {
        void* mem = mmap(nullptr, sizeof(std::atomic<bool>), // NOLINT(misc-const-correctness)
            PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (mem == MAP_FAILED) {
            return;
        }
        stopFlag_ = new (mem) std::atomic<bool>(false); // NOLINT(cppcoreguidelines-owning-memory)

        pid_ = fork();
        if (pid_ == 0) {
            bool result = false;
            if constexpr (requires { std::forward<Func>(func)(stop_token { stopFlag_ }); }) {
                result = std::forward<Func>(func)(stop_token { stopFlag_ });
            } else {
                result = std::forward<Func>(func)();
            }
            _exit(result ? 0 : 1);
        }
    }

    jprocess(const jprocess&) = delete;
    jprocess& operator=(const jprocess&) = delete;

    jprocess(jprocess&& other) noexcept;
    jprocess& operator=(jprocess&& other) noexcept;

    ~jprocess();

    void request_stop() noexcept;

    // Waits for the child to exit; returns true iff it exited with status 0.
    // Safe to call multiple times; subsequent calls return false immediately.
    bool join() noexcept;

    // Non-blocking check. Returns the exit result if the child already exited,
    // or std::nullopt if it is still running. Marks as joined on success so that
    // a subsequent join() returns immediately without a second waitpid.
    std::optional<bool> tryJoin() noexcept;

    [[nodiscard]] stop_token get_stop_token() noexcept;

    [[nodiscard]] pid_t pid() const noexcept { return pid_; }

private:
    void stopAndJoin() noexcept;
    void freeSharedMemory() noexcept;

    std::atomic<bool>* stopFlag_ { nullptr };
    pid_t pid_ { -1 };
    bool joined_ { false };
};

#endif // __linux__
