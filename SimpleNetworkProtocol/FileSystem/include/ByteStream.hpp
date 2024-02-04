#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <type_traits>

template <class T>
concept trivial = std::is_trivial_v<T>;

class OutputByteStream {
public:
    OutputByteStream(std::basic_ostream<std::byte>& stream);

    OutputByteStream& Write(void* pointer, std::size_t size);

private:
    std::reference_wrapper<std::basic_ostream<std::byte>> _outStream;

    template <class T>
    friend OutputByteStream& operator<<(OutputByteStream& stream, const std::basic_string<T>& string);
    template <trivial T>
    friend OutputByteStream& operator<<(OutputByteStream& stream, const T& trivial);
};

class InputByteStream {
public:
    InputByteStream(std::basic_istream<std::byte>& stream);

    InputByteStream& Read(void* pointer, std::size_t size);

private:
    std::reference_wrapper<std::basic_istream<std::byte>> _inStream;

    template <class T>
    friend InputByteStream& operator>>(InputByteStream& stream, std::basic_string<T>& string);
    template <trivial T>
    friend InputByteStream& operator>>(InputByteStream& stream, T& trivial);
};

template <class T>
OutputByteStream& operator<<(OutputByteStream& stream, const std::basic_string<T>& string)
{
    std::uint32_t size = string.size();
    stream._outStream.get().write((std::byte*)(&size), sizeof(size));
    stream._outStream.get().write((std::byte*)string.c_str(), string.size());
    return stream;
}

template <class T>
InputByteStream& operator>>(InputByteStream& stream, std::basic_string<T>& string)
{
    std::uint32_t size;
    stream._inStream.get().read((std::byte*)&size, sizeof(size));
    string.resize(size);
    stream._inStream.get().read((std::byte*)string.data(), size);
    return stream;
}

template <trivial T>
OutputByteStream& operator<<(OutputByteStream& stream, const T& trivial)
{
    stream._outStream.get().write((std::byte*)(&trivial), sizeof(trivial));
    return stream;
}

template <trivial T>
InputByteStream& operator>>(InputByteStream& stream, T& trivial)
{
    stream._inStream.get().read((std::byte*)&trivial, sizeof(trivial));
    return stream;
}

OutputByteStream& operator<<(OutputByteStream& stream, const std::filesystem::path& path);
InputByteStream& operator>>(InputByteStream& stream, std::filesystem::path& path);
