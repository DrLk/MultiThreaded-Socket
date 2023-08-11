#pragma once

#include <atomic>
#include <string_view>
#include <unordered_map>

namespace FastTransport::Performance {
class Counter {
    static std::unordered_map<std::string, Counter&> Counters;

    std::atomic<uint64_t> _counter;
    std::string _name;

public:
    Counter(std::string_view name);
    ~Counter();
    void Count();

    static void Print();
};
} // namespace FastTransport::Performance
