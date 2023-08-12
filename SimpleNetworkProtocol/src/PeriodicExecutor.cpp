#include "PeriodicExecutor.hpp"

#include <chrono>
#include <compare>
#include <ratio>
#include <thread>
#include <utility>

namespace FastTransport::Protocol {
using namespace std::chrono_literals;

PeriodicExecutor::PeriodicExecutor(std::function<void()> function, const std::chrono::microseconds& interval)
    : _function(std::move(function))
    , _interval(interval)
    , _start(0)
{
}

void PeriodicExecutor::Run()
{
    while (true) {
        if (_end.time_since_epoch().count() == 0) {
            break;
        }

        auto now = clock::now();
        auto waitInterval = now - _end;
        if ((_interval - waitInterval) > 10ms) {
            const auto& interval = _interval - waitInterval;
            std::this_thread::sleep_for(interval);
        } else {
            while ((clock::now() - _end) < _interval) {
            }
        }

        break;
    }

    _end = clock::now();
    RunFunction();
}

void PeriodicExecutor::RunFunction()
{
    if (_function) {
        _function();
    }
}
} // namespace FastTransport::Protocol
