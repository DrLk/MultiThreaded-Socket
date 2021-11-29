#pragma once

#include <functional>
#include <chrono>


namespace FastTransport
{
    namespace   Protocol
    {

        class PeriodicExecutor
        {
        public:
            PeriodicExecutor(std::function<void()>, const std::chrono::microseconds& interval);

            void Run();

        private:
            typedef std::chrono::steady_clock clock;

            std::function<void()> _function;

            std::chrono::microseconds _interval;
            std::chrono::microseconds _start;
            clock::time_point _end;

            void RunFunction();

        };
    }
}