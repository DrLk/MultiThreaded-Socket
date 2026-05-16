#pragma once

#include <cstdint>

#include "Inode.hpp"
#include "Leaf.hpp"

namespace FastTransport::TaskQueue {

// Translates a client-side FUSE inode (= client Leaf* address) to the
// server-side inode stored in that Leaf.  Used in Request*Job classes before
// serialising the inode into a network message.
// RootInode is the same on both sides and is passed through unchanged.
inline std::uint64_t ToServerInode(std::uint64_t inode)
{
    if (inode == FileSystem::RootInode) {
        return FileSystem::RootInode;
    }
    return reinterpret_cast<const FileSystem::Leaf*>(inode)->GetServerInode(); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}

} // namespace FastTransport::TaskQueue
