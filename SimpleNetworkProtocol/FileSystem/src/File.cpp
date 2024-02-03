#include "File.hpp"

#include <filesystem>
#include <iostream>

namespace FastTransport::FileSystem {

void File::Serialize(std::ostream& stream) const
{
    stream << name;
    stream << size;
    stream << (unsigned char)type;
}

void File::Deserialize(std::istream& stream)
{
    stream >> name;
    stream >> size;
    unsigned char byte;
    stream >> byte;
    type = (std::filesystem::file_type)type;
}

} // namespace FastTransport::FileSystem
