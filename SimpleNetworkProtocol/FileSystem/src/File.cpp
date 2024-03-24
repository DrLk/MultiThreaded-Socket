#include "File.hpp"

#include <cstdint>
#include <filesystem>

namespace FastTransport::FileSystem {

File::File()
    : File("", 0, std::filesystem::file_type::none)
{
}

File::File(const std::filesystem::path& name, std::uintmax_t size, std::filesystem::file_type type)
    : name(name)
    , size(size)
    , type(type)
{
}

File::~File() = default;

} // namespace FastTransport::FileSystem
