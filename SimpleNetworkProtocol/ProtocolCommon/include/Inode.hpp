#pragma once

#include <cstdint>

namespace FastTransport::FileSystem {

// Identical to FUSE_ROOT_ID from <fuse3/fuse_lowlevel.h>. Hardcoded so this
// header (and code that uses it) does not have to drag in libfuse.
inline constexpr std::uint64_t RootInode = 1;

} // namespace FastTransport::FileSystem
