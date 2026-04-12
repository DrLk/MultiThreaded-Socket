#include "jprocess.hpp"

#ifdef __linux__

jprocess::jprocess(jprocess&& other) noexcept
    : stopFlag_(std::exchange(other.stopFlag_, nullptr))
    , pid_(std::exchange(other.pid_, -1))
    , joined_(std::exchange(other.joined_, true))
{
}

jprocess& jprocess::operator=(jprocess&& other) noexcept
{
    if (this != &other) {
        stopAndJoin();
        freeSharedMemory();
        stopFlag_ = std::exchange(other.stopFlag_, nullptr);
        pid_ = std::exchange(other.pid_, -1);
        joined_ = std::exchange(other.joined_, true);
    }
    return *this;
}

jprocess::~jprocess()
{
    stopAndJoin();
    freeSharedMemory();
}

void jprocess::request_stop() noexcept
{
    if (stopFlag_ != nullptr) {
        stopFlag_->store(true, std::memory_order_relaxed);
    }
}

bool jprocess::join() noexcept
{
    if (pid_ <= 0 || joined_) {
        return false;
    }
    joined_ = true;
    int status = 0;
    waitpid(pid_, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

std::optional<bool> jprocess::tryJoin() noexcept
{
    if (pid_ <= 0 || joined_) {
        return false;
    }
    int status = 0;
    const pid_t result = waitpid(pid_, &status, WNOHANG);
    if (result == 0) {
        return std::nullopt; // still running
    }
    joined_ = true;
    if (result < 0) {
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

jprocess::stop_token jprocess::get_stop_token() noexcept
{
    return stop_token { stopFlag_ };
}

void jprocess::stopAndJoin() noexcept
{
    if (pid_ > 0 && !joined_) {
        request_stop();
        join();
    }
}

void jprocess::freeSharedMemory() noexcept
{
    if (stopFlag_ != nullptr) {
        munmap(stopFlag_, sizeof(std::atomic<bool>));
        stopFlag_ = nullptr;
    }
}

#endif // __linux__
