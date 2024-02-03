#include "File.hpp"

#include <filesystem>
#include <iostream>

namespace FastTransport::FileSystem {

void File::Serialize(std::ostream& stream) const
{
    std::uint32_t size = name.u8string().size();
    stream.write((const char*)(&size), sizeof(size));
    stream.write((char*)name.u8string().c_str(), name.u8string().size());
    stream.write((char*)&size, sizeof(size));
    stream.write((char*)&type, sizeof(type));
}

void File::Deserialize(std::istream& stream)
{
    std::uint32_t size;
    stream.read((char*)&size, sizeof(size));
    std::u8string u8path;
    u8path.resize(size);
    stream.read((char*)u8path.data(), size);
    name = u8path;
    stream.read((char*)&size, sizeof(size));
    stream.read((char*)&type, sizeof(type));
}

} // namespace FastTransport::FileSystem
