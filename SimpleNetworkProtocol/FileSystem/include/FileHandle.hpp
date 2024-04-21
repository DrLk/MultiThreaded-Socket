#pragma once

#include "LocalFileHandle.hpp"
#include "RemoteFileHandle.hpp"

namespace FastTransport::FileSystem {

struct FileHandle {
    LocalFileHandle localFile;
    RemoteFileHandle* remoteFile;
} __attribute__((aligned(16)));

} // namespace FastTransport::FileSystem
