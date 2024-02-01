#include "FileSystem.hpp"

#include <cstring>
#include <errno.h>
#include <fuse3/fuse.h>

#include "File.hpp"
#include "FileTree.hpp"

namespace FastTransport::Protocol {
std::unordered_map<fuse_ino_t, std::reference_wrapper<Leaf>> FileSystem::_openedFiles;
FileTree FileSystem::_tree = FileTree::GetTestFileTree();

} // namespace FastTransport::Protocol
