#pragma once

#include <cstddef>
#include <functional>

struct MemoryKeyHash {
    std::size_t operator()(const std::pair<std::size_t, std::size_t>& pair) const // NOLINT(fuchsia-overloaded-operator)
    {
        auto firstHash = std::hash<std::size_t> {}(pair.first);
        auto secondHash = std::hash<std::size_t> {}(pair.second);

        // Mainly for demonstration purposes, i.e. works but is overly simple
        // In the real world, use sth. like boost.hash_combine
        auto result = firstHash ^ secondHash;
        return result;
    }
};