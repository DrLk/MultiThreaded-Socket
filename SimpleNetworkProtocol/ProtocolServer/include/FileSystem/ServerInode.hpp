#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "Leaf.hpp"

namespace FastTransport::TaskQueue {

// Translates a client-side FUSE inode (= client Leaf* address) to the
// server-side inode stored in that Leaf.  Used in Request*Job classes before
// serialising the inode into a network message.
// FUSE_ROOT_ID is the same on both sides and is passed through unchanged.
inline fuse_ino_t ToServerInode(fuse_ino_t inode)
{
    if (inode == FUSE_ROOT_ID) {
        return FUSE_ROOT_ID;
    }
    return reinterpret_cast<const FileSystem::Leaf*>(inode)->GetServerInode(); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}

} // namespace FastTransport::TaskQueue
