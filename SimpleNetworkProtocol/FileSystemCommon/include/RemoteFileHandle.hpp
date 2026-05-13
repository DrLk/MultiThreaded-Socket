#pragma once

#include "File.hpp"

namespace FastTransport::FileSystem {

struct RemoteFileHandle {
    std::unique_ptr<File> file2;
} __attribute__((aligned(8)));

} // namespace FastTransport::FileSystem
