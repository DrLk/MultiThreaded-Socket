#pragma once

#include <cstddef>

namespace FastTransport::FileSystem {

template <typename T>
concept OutputStream = requires(T stream, std::byte* data, std::ptrdiff_t size) {
    {
        stream.write(data, size)
    };
    {
        stream.flush()
    };
};

template <typename T>
concept InputStream = requires(T stream, std::byte* data, std::ptrdiff_t size) {
    {
        stream.read(data, size)
    };
};
} // namespace FastTransport::FileSystem
