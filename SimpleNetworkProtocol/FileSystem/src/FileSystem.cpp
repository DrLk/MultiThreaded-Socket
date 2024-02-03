#include "FileSystem.hpp"

#include <functional>
#include <fuse3/fuse.h>
#include <unordered_map>

#include "FileTree.hpp"

namespace FastTransport::FileSystem {
std::unordered_map<fuse_ino_t, std::reference_wrapper<Leaf>> FileSystem::_openedFiles;
FileTree FileSystem::_tree = FileTree::GetTestFileTree();

} // namespace FastTransport::FileSystem
