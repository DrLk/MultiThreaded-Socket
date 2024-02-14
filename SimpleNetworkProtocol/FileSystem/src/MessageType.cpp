#include "MessageType.hpp"

#include <cstdint>

FastTransport::FileSystem::OutputByteStream& operator<<(FastTransport::FileSystem::OutputByteStream& stream, const RequestTree& message)
{
    stream << message.path;
    return stream;
}
