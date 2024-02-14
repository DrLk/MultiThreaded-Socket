#include "ByteStream.hpp"

namespace FastTransport::FileSystem {

OutputByteStream::OutputByteStream(std::basic_ostream<std::byte>& stream)
    : _outStream(stream)
{
}

OutputByteStream& OutputByteStream::Write(void* pointer, std::size_t size)
{
    _outStream.get().write((std::byte*)pointer, size);
    return *this;
}

OutputByteStream& operator<<(OutputByteStream& stream, const std::filesystem::path& path)
{
    stream << path.u8string();
    return stream;
}

} // namespace FastTransport::FileSystem
