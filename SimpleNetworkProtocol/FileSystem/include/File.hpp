#pragma once

#include <cstdint>
#include <filesystem>

struct File {
    std::filesystem::path name;
    std::uintmax_t size;
    std::filesystem::file_type type;

    void Serialize(std::ostream& stream)
    {
        stream << name;
        stream << size;
        stream << (unsigned char)type;
    }

    void Deserialize(std::istream& stream)
    {
        stream >> name;
        stream >> size;
        unsigned char byte;
        stream >> byte;
        type = (std::filesystem::file_type)type;
    }
};
