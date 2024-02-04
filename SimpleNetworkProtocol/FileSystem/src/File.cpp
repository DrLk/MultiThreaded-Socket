#include "File.hpp"

#include <filesystem>

namespace FastTransport::FileSystem {

void File::Serialize(OutputByteStream& stream) const
{
    stream << name;
    stream << size;
    stream << type;
}

void File::Deserialize(InputByteStream& stream)
{
    stream >> name;
    stream >> size;
    stream >> type;
}

} // namespace FastTransport::FileSystem
