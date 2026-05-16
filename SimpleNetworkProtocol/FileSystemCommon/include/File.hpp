#pragma once

#include <cstdint>
#include <filesystem>

#include "ByteStream.hpp"
#include "IPacket.hpp"

struct stat;

namespace FastTransport::FileSystem {

class File {
private:
    using IPacket = FastTransport::Protocol::IPacket;

public:
    File();
    File(const File& that) = delete;
    File(File&& that) noexcept = default;
    File(std::uint64_t size, std::filesystem::file_type type);
    File& operator=(const File& that) = delete;
    File& operator=(File&& that) noexcept = default;
    virtual ~File();

    template <OutputStream Stream>
    void Serialize(OutputByteStream<Stream>& stream) const
    {
        stream << size;
        stream << type;
    }

    template <InputStream Stream>
    void Deserialize(InputByteStream<Stream>& stream)
    {
        stream >> size;
        stream >> type;
    }

    virtual void Open() = 0;
    virtual void Create() = 0;
    [[nodiscard]] virtual bool IsOpened() const = 0;
    virtual int Close() = 0;
    virtual int Stat(struct stat& stat) = 0;
    virtual IPacket::List Read(IPacket::List& packets, size_t size, off_t offset) = 0;
    virtual void Write(IPacket::List& packets, size_t size, off_t offset) = 0;
    virtual void Write(IPacket::List&& packets, size_t size, off_t offset) = 0;

    [[nodiscard]] std::uint64_t GetSize() const;
    [[nodiscard]] std::filesystem::file_type GetType() const;

private:
    std::uint64_t size;
    std::filesystem::file_type type;
};

} // namespace FastTransport::FileSystem
