#pragma once

#include <atomic>
#include <string>
#include <string_view>
#include <unordered_map>

namespace FastTransport::Performance {
class Counter {
    static std::unordered_map<std::string, Counter&> Counters; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    std::atomic<uint64_t> _counter;
    std::string _name;

public:
    explicit Counter(std::string_view name);
    ~Counter();
    Counter(const Counter& that) = delete;
    Counter(Counter&& that) = delete;
    Counter& operator=(const Counter& that) = delete;
    Counter& operator=(Counter&& that) = delete;
    void Count();

    static void Print();
};
} // namespace FastTransport::Performance
