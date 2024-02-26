#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <type_traits>

namespace FastTransport::FileSystem {

template <typename T>
concept OutputStream = requires(T stream, std::byte* data, std::ptrdiff_t size) {
    {
        stream.write(data, size)
    };
    {
        stream.flush()
    };
};

template <typename T>
concept InputStream = requires(T stream, std::byte* data, std::ptrdiff_t size) {
    {
        stream.read(data, size)
    };
};

template <class T>
concept trivial = std::is_trivial_v<T>;

template <OutputStream Stream>
class OutputByteStream {
public:
    explicit OutputByteStream(Stream& stream)
        : _outStream(stream)
    {
    }

    OutputByteStream& Write(void* pointer, std::size_t size)
    {
        _outStream.get().write(static_cast<std::byte*>(pointer), size);
        return *this;
    }

    OutputByteStream& Flush()
    {
        _outStream.get().flush();
        return *this;
    }

    template <class T>
    OutputByteStream& operator<<(const std::basic_string<T>& string)
    {
        std::uint32_t size = string.size();
        _outStream.get().write(reinterpret_cast<std::byte*>(&size), sizeof(size));
        _outStream.get().write((std::byte*)string.c_str(), string.size());
        return *this;
    }

    template <trivial T>
    OutputByteStream& operator<<(const T& trivial)
    {
        _outStream.get().write((std::byte*)(&trivial), sizeof(trivial));
        return *this;
    }

    OutputByteStream& operator<<(const std::filesystem::path& path)
    {
        operator<<(path.u8string());
        return *this;
    }

private:
    std::reference_wrapper<Stream> _outStream {};
};

template <FastTransport::FileSystem::InputStream Stream>
class InputByteStream {
public:
    explicit InputByteStream(Stream& stream)
        : _inStream(stream)
    {
    }

    InputByteStream& Read(void* pointer, std::size_t size)
    {
        _inStream.get().read(static_cast<std::byte*>(pointer), size);
        return *this;
    }

    template <trivial T>
    InputByteStream<Stream>& operator>>(T& trivial)
    {
        _inStream.get().read((std::byte*)&trivial, sizeof(trivial));
        return *this;
    }
    template <class T>
    InputByteStream<Stream>& operator>>(std::basic_string<T>& string)
    {
        std::uint32_t size = 0;
        _inStream.get().read(reinterpret_cast<std::byte*>(&size), sizeof(size));
        string.resize(size);
        _inStream.get().read((std::byte*)string.data(), size);
        return *this;
    }

    InputByteStream<Stream>& operator>>(std::filesystem::path& path)
    {
        std::u8string string;
        operator>>(string);
        path = string;
        return *this;
    }

private:
    std::reference_wrapper<Stream> _inStream {};
};

} // namespace FastTransport::FileSystem
