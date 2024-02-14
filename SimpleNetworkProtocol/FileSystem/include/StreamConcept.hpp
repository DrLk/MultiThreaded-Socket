#pragma once

#include <concepts>
#include <cstddef>
#include <string>

namespace FastTransport::FileSystem {

template <typename T>
concept OutputStream = requires(T stream, std::byte* data, std::ptrdiff_t size) {
    {
        stream.write(data, size)
    };
};

template <typename T>
concept InputStream = requires(T stream, std::byte* data, std::ptrdiff_t size) {
    {
        stream >> size
    };
    {
        stream.read(data, size)
    };
    /* } -> std::same_as<T&>; */
};
} // namespace FastTransport::FileSystem
