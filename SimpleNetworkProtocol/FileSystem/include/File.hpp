#pragma once

#include <cstdint>
#include <filesystem>

struct File {
    std::filesystem::path name;
    std::uintmax_t size;
    std::filesystem::file_type type;
};
