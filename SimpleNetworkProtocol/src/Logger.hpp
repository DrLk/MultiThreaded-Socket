#pragma once

#include <array>
#include <chrono>
#include <format>
#include <iostream>
#include <memory_resource>
#include <syncstream>

#define LOGGER \
    if (2 > 1) \
    FastTransport::Logger::LogHelper

namespace FastTransport::Logger {

class LogHelper {
public:
    LogHelper()
        : _memory()
        , _buffer(_memory.data(), _memory.size(), std::pmr::new_delete_resource())
        , _osyncstream(std::cout, std::pmr::polymorphic_allocator<char>(&_buffer))
    {
        auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
        _osyncstream << "[" << std::format("{:%Y-%m-%d %H:%M:%OS}", now) << "] ";
    }

    ~LogHelper()
    {
        _osyncstream << std::endl;
    }

    LogHelper(const LogHelper&) = delete;
    LogHelper(LogHelper&&) = delete;
    LogHelper& operator=(const LogHelper&) = delete;
    LogHelper& operator=(LogHelper&&) = delete;

    template <class T>
    LogHelper& operator<<(T&& value) // NOLINT(fuchsia-overloaded-operator)
    {
        _osyncstream << value;
        return *this;
    }

private:
    std::array<char, 1024 * sizeof(int)> _memory {};
    std::pmr::monotonic_buffer_resource _buffer;
    std::basic_osyncstream<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>> _osyncstream;
};
} // namespace FastTransport::Logger