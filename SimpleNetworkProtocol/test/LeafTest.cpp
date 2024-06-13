#include "Leaf.hpp"
#include <cstddef>
#include <gtest/gtest.h>

#include <sstream>

#include "ByteStream.hpp"
#include "LeafSerializer.hpp"
#include "Packet.hpp"

namespace {

using FastTransport::FileSystem::Leaf;

Leaf GetTestLeaf()
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

void CompareLeaf(const Leaf& left, const Leaf& right) // NOLINT(misc-no-recursion)
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

} // namespace

namespace FastTransport::FileSystem {

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

TEST(LeafTest, LeafGetDataTest)
{
    Leaf root = GetTestLeaf();

    Protocol::IPacket::List data;
    constexpr size_t PacketSize = 1400;
    std::array<std::byte, PacketSize> buffer {};
    char value = 0;
    for (auto& byte : buffer) {
        byte = std::byte(value++);
    }

    constexpr size_t PacketNumber = 200; 
    for (int i = 0; i < PacketNumber; i++) {
        auto packet = std::make_unique<Protocol::Packet>(1500);
        packet->SetPayload(buffer);
        data.push_back(std::move(packet));
    }

    root.AddData(0, PacketNumber * PacketSize, std::move(data));

    auto buffer2 = root.GetData(0, 1400);
    EXPECT_EQ(buffer2->count, 1);
    std::span<std::byte> span(static_cast<std::byte*>(buffer2->buf[0].mem), buffer2->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer.begin(), buffer.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                                                                  // j
                                                                                  //
    auto buffer3 = root.GetData(1, 1399);
    EXPECT_EQ(buffer3->count, 1);
    span = std::span<std::byte>(static_cast<std::byte*>(buffer3->buf[0].mem), buffer3->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer.begin() + 1, buffer.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                                                                      //
    auto buffer4 = root.GetData(0, 1399);
    EXPECT_EQ(buffer4->count, 1);
    span = std::span<std::byte>(static_cast<std::byte*>(buffer4->buf[0].mem), buffer4->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer.begin(), buffer.end() - 1)); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                                                                      //
    auto buffer5 = root.GetData(1, 2798);
    EXPECT_EQ(buffer5->count, 2);
    span = std::span<std::byte>(static_cast<std::byte*>(buffer5->buf[0].mem), buffer5->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer.begin() + 1, buffer.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    span = std::span<std::byte>(static_cast<std::byte*>(buffer5->buf[1].mem), buffer5->buf[1].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer.begin(), buffer.end() - 1)); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                                                                      //
    auto buffer6 = root.GetData(1401, 2798);
    EXPECT_EQ(buffer6->count, 2);
    span = std::span<std::byte>(static_cast<std::byte*>(buffer6->buf[0].mem), buffer6->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer.begin() + 1, buffer.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    span = std::span<std::byte>(static_cast<std::byte*>(buffer6->buf[1].mem), buffer6->buf[1].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer.begin(), buffer.end() - 1)); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    int iii = 0;
    iii++;
}

} // namespace FastTransport::FileSystem
