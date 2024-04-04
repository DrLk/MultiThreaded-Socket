#pragma once

#include <memory>

#include "ByteStream.hpp"
#include "File.hpp"
#include "Leaf.hpp"
#include "NativeFile.hpp"

namespace FastTransport::FileSystem {
class LeafSerializer final {
    using FilePtr = std::unique_ptr<File>;

public:
    template <OutputStream Stream>
    static void Serialize(const Leaf& leaf, OutputByteStream<Stream>& stream) // NOLINT(misc-no-recursion)
    {
        stream << leaf.GetName();
        stream << leaf.GetType();
        const int size = leaf.children.size();
        stream << size;
        for (const auto& [name, child] : leaf.children) {
            Serialize(child, stream);
        }
    }

    template <InputStream Stream>
    static Leaf Deserialize(InputByteStream<Stream>& stream, Leaf* parent)
    {
        std::filesystem::path name;
        stream >> name;
        std::filesystem::file_type type;
        stream >> type;
        FilePtr file(new NativeFile());
        Leaf leaf(name, type, parent);
        leaf.SetFile(std::move(file));

        int size;
        stream >> size;
        for (int i = 0; i < size; i++) {
            Leaf childLeaf = Deserialize(stream, &leaf);
            leaf.children.insert({ childLeaf.GetName().native(), std::move(childLeaf) });
        }

        return leaf;
    }
};

} // namespace FastTransport::FileSystem
