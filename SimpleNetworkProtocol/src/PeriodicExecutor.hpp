#pragma once

#include <chrono>
#include <functional>

namespace FastTransport::Protocol {

class PeriodicExecutor {
    using clock = std::chrono::steady_clock;

public:
    PeriodicExecutor(std::function<void()> /*function*/, const std::chrono::microseconds& interval);

    void Run();

private:
    std::function<void()> _function;

    std::chrono::microseconds _interval;
    std::chrono::microseconds _start;
    clock::time_point _end;

    void RunFunction();
};
} // namespace FastTransport::Protocol