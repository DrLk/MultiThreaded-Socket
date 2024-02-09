#include "ByteStream.hpp"

OutputByteStream::OutputByteStream(std::basic_ostream<std::byte>& stream)
    : _outStream(stream)
{
}

OutputByteStream& OutputByteStream::Write(void* pointer, std::size_t size)
{
    _outStream.get().write((std::byte*)pointer, size);
    return *this;
}

InputByteStream::InputByteStream(std::basic_istream<std::byte>& stream)
    : _inStream(stream)
{
}

InputByteStream& InputByteStream::Read(void* pointer, std::size_t size)
{
    _inStream.get().read((std::byte*)pointer, size);
    return *this;
}

OutputByteStream& operator<<(OutputByteStream& stream, const std::filesystem::path& path)
{
    stream << path.u8string();
    return stream;
}

InputByteStream& operator>>(InputByteStream& stream, std::filesystem::path& path)
{
    std::u8string string;
    stream >> string;
    path = string;
    return stream;
}
