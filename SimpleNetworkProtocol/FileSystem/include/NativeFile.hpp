#pragma once

#include "File.hpp"
#include "IPacket.hpp"

namespace FastTransport::FileSystem {

class NativeFile : public File {
    using IPacket = FastTransport::Protocol::IPacket;
public:
    NativeFile();
    NativeFile(const std::filesystem::path& name, std::uint64_t size, std::filesystem::file_type type);

    void Open() override;
    IPacket::List Read(IPacket::List& packets, size_t size, off_t offset) override;
    void Write(IPacket::List& packets, size_t size, off_t offset) override;

private:
    int _file;
};

} // namespace FastTransport::FileSystem
