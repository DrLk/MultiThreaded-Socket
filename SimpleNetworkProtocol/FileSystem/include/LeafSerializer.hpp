#pragma once

#include <memory>

#include "ByteStream.hpp"
#include "File.hpp"
#include "Leaf.hpp"

namespace FastTransport::FileSystem {
class LeafSerializer final {
    using FilePtr = std::unique_ptr<File>;

public:
    template <OutputStream Stream>
    static void Serialize(const Leaf& leaf, OutputByteStream<Stream>& stream) // NOLINT(misc-no-recursion)
    {
        stream << leaf.GetName();
        stream << leaf.GetType();
        const std::uint64_t size = leaf.GetChildren().size();
        stream << size;
        for (const auto& [name, child] : leaf.GetChildren()) {
            Serialize(child, stream);
        }
    }

    template <InputStream Stream>
    static Leaf Deserialize(InputByteStream<Stream>& stream, Leaf* parent) // NOLINT(misc-no-recursion)
    {
        std::filesystem::path name;
        stream >> name;
        std::filesystem::file_type type {};
        stream >> type;
        Leaf leaf(std::move(name), type, parent);

        std::uint64_t size = 0;
        stream >> size;
        for (int i = 0; i < size; i++) {
            Leaf childLeaf = Deserialize(stream, &leaf);
            leaf.AddChild(std::move(childLeaf));
        }

        return leaf;
    }
};

} // namespace FastTransport::FileSystem
