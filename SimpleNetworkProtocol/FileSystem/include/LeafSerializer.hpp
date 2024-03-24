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
    static void Serialize(const Leaf& leaf, OutputByteStream<Stream>& stream)
    {
        leaf.GetFile().Serialize(stream);
        const int size = leaf.children.size();
        stream << size;
        for (const auto& [name, child] : leaf.children) {
            Serialize(child, stream);
        }
    }

    template <InputStream Stream>
    static void Deserialize(Leaf& leaf, InputByteStream<Stream>& stream, Leaf* parent)
    {
        leaf.parent = parent;
        FilePtr file(new NativeFile());
        file->Deserialize(stream);
        leaf.SetFile(std::move(file));
        const int size = 0;
        stream >> size;
        for (int i = 0; i < size; i++) {
            Leaf childLeaf;
            Deserialize(childLeaf, stream, &leaf);
            leaf.children.insert({ childLeaf.GetFile().GetName().native(), std::move(childLeaf) });
        }
    }
};

} // namespace FastTransport::FileSystem
