#pragma once

namespace FastTransport::FileSystem {

struct FileHandle {
    int localFile;
    int remoteFile;
} __attribute__((aligned(8)));

} // namespace FastTransport::FileSystem
