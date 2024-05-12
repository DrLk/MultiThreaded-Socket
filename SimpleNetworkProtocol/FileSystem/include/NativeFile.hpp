#pragma once

#include <cstddef>

#include "File.hpp"
#include "IPacket.hpp"

namespace FastTransport::FileSystem {

class NativeFile : public File {
    using IPacket = FastTransport::Protocol::IPacket;

public:
    explicit NativeFile(const std::filesystem::path& name);

    void Open() override;
    [[nodiscard]] bool IsOpened() const override;
    int Close() override;
    int Stat(struct stat& stat) override;
    std::size_t Read(IPacket::List& packets, std::size_t size, off_t offset) override;
    void Write(IPacket::List& packets, size_t size, off_t offset) override;

private:
    int _file {-1};
};

} // namespace FastTransport::FileSystem
