#include "MessageType.hpp"
#include <cstdint>

OutputByteStream& operator<<(OutputByteStream& stream, MessageType message)
{
    stream.write((char*)&message, sizeof(message));
    return stream;
}

InputByteStream& operator>>(InputByteStream& stream, MessageType& message)
{
    stream.read((char*)&message, sizeof(message));
    return stream;
}

OutputByteStream& operator<<(OutputByteStream& stream, const RequestTree& message)
{
    std::uint32_t size = message.path.size();
    stream.write((const char*)(&size), sizeof(size));
    stream.write(message.path.c_str(), message.path.size());
    return stream;
}

InputByteStream& operator>>(InputByteStream& stream, RequestTree& message)
{
    std::uint32_t size;
    stream.read((char*)&size, sizeof(size));
    message.path.resize(size);
    stream.read(message.path.data(), size);
    return stream;
}
