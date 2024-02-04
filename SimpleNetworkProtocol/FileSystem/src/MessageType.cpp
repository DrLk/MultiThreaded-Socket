#include "MessageType.hpp"

#include <cstdint>


OutputByteStream& operator<<(OutputByteStream& stream, const RequestTree& message)
{
    stream << message.path;
    return stream;
}

InputByteStream& operator>>(InputByteStream& stream, RequestTree& message)
{
    stream >> message.path;
    return stream;
}
