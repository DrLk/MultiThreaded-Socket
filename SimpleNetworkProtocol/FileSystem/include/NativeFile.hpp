#pragma once

#include <cstddef>

#include "File.hpp"
#include "IPacket.hpp"

namespace FastTransport::FileSystem {

class NativeFile : public File {
    using IPacket = FastTransport::Protocol::IPacket;

public:
    using Ptr = std::shared_ptr<NativeFile>;

    explicit NativeFile(std::filesystem::path name);
    ~NativeFile() override;
    NativeFile(const NativeFile&) = delete;
    NativeFile(NativeFile&&) = default;
    NativeFile& operator=(const NativeFile&) = delete;
    NativeFile& operator=(NativeFile&&) = default;

    [[nodiscard]] int GetHandle() const;
    void Open() override;
    void Create() override;
    [[nodiscard]] bool IsOpened() const override;
    int Close() override;
    int Stat(struct stat& stat) override;
    IPacket::List Read(IPacket::List& packets, std::size_t size, off_t offset) override;
    void Write(IPacket::List& packets, size_t size, off_t offset) override;
    void Write(IPacket::List&& packets, size_t size, off_t offset) override;

private:
    std::filesystem::path _name;
    int _file {-1};
};

} // namespace FastTransport::FileSystem
