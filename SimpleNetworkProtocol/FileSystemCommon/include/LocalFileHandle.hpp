#pragma once

namespace FastTransport::FileSystem {

struct LocalFileHandle {
    int file;
} __attribute__((aligned(4)));

} // namespace FastTransport::FileSystem
