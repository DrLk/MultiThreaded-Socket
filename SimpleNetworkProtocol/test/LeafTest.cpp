#include "Leaf.hpp"
#include <gtest/gtest.h>

#include <sstream>

#include "ByteStream.hpp"
#include "LeafSerializer.hpp"

namespace FastTransport::FileSystem {

static Leaf GetTestLeaf()
{
    Leaf root("test", std::filesystem::file_type::directory, nullptr);

    root.AddChild(
            "folder1",
            std::filesystem::file_type::directory)
        .AddChild(
            "file1",
            std::filesystem::file_type::regular);

    Leaf& folder2 = root.AddChild(
        "folder2",
        std::filesystem::file_type::directory);

    folder2.AddChild(
        "file2_1",
        std::filesystem::file_type::regular);
    folder2.AddChild(
        "file2_2",
        std::filesystem::file_type::regular);

    return root;
}

static void CompareLeaf(const Leaf& left, const Leaf& right) // NOLINT(misc-no-recursion)
{
    EXPECT_EQ(left.GetName(), right.GetName());
    EXPECT_EQ(left.GetType(), right.GetType());
    EXPECT_EQ(left.GetChildren().size(), right.GetChildren().size());


    for (const auto& [name, child] : left.GetChildren()) {
        const auto& rightChild = right.GetChildren().find(name);
        if (rightChild == right.GetChildren().end()) {
            FAIL() << "Child not found: " << name;
            break;
        }

        CompareLeaf(child, rightChild->second);
    }
}

TEST(LeafTest, LeafSerializationTest)
{
    const Leaf root = GetTestLeaf();

    std::basic_stringstream<std::byte> stream;
    OutputByteStream<std::basic_stringstream<std::byte>> out(stream);

    LeafSerializer::Serialize(root, out);

    InputByteStream<std::basic_stringstream<std::byte>> input(stream);
    const Leaf deserializedRoot = LeafSerializer::Deserialize(input, nullptr);

    CompareLeaf(root, deserializedRoot);
}

} // namespace FastTransport::FileSystem
