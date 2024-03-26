#pragma once

#include "File.hpp"

namespace FastTransport::FileSystem {

class NativeFile : public File {
public:
    NativeFile()
        : NativeFile("", 0, std::filesystem::file_type::none)
    {
    }

    NativeFile(const std::filesystem::path& name, std::uint64_t size, std::filesystem::file_type type)
        : File(name, size, type)
    {
    }

    void Read() override
    {
        // Read file
    }
};

} // namespace FastTransport::FileSystem
