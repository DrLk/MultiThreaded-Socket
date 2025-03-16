#include "Leaf.hpp"
#include <gtest/gtest.h>

#include <cstddef>
#include <sstream>

#include "ByteStream.hpp"
#include "LeafSerializer.hpp"
#include "Packet.hpp"

namespace {

using FastTransport::FileSystem::Leaf;

Leaf GetTestLeaf()
{
    Leaf root("test", std::filesystem::file_type::directory, 0, nullptr);

    root.AddChild(
            "folder1",
            std::filesystem::file_type::directory, 0)
        .AddChild(
            "file1",
            std::filesystem::file_type::regular, 100);

    Leaf& folder2 = root.AddChild(
        "folder2",
        std::filesystem::file_type::directory, 0);

    folder2.AddChild(
        "file2_1",
        std::filesystem::file_type::regular, 200);
    folder2.AddChild(
        "file2_2",
        std::filesystem::file_type::regular, 300);

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

FastTransport::Protocol::IPacket::List GetTestData(size_t size, char value)
{
    FastTransport::Protocol::IPacket::List data;
    std::array<std::byte, 1400> buffer {};
    for (auto& byte : buffer) {
        byte = std::byte(value);
    }

    while (size > 0) {
        auto packet = std::make_unique<FastTransport::Protocol::Packet>(1500);
        packet->SetPayload(buffer);
        packet->SetPayloadSize(std::min(size, buffer.size()));
        size -= packet->GetPayload().size();
        data.push_back(std::move(packet));
    }

    return data;
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
}

TEST(LeafTest, LeafAddDataTest)
{
    Leaf root = GetTestLeaf();

    Protocol::IPacket::List data;
    constexpr size_t PacketSize = 1400;
    std::array<std::byte, PacketSize> buffer0 {};
    for (auto& byte : buffer0) {
        byte = std::byte(0);
    }
    auto packet0 = std::make_unique<Protocol::Packet>(1500);
    packet0->SetPayload(buffer0);
    data.push_back(std::move(packet0));

    root.AddData(0, PacketSize, std::move(data));

    Protocol::IPacket::List data2;
    std::array<std::byte, PacketSize> buffer1 {};
    for (auto& byte : buffer1) {
        byte = std::byte(1);
    }
    auto packet1 = std::make_unique<Protocol::Packet>(1500);
    packet1->SetPayload(buffer1);
    data.push_back(std::move(packet1));

    root.AddData(1400, PacketSize, std::move(data));

    auto result1 = root.GetData(0, 2800);

    EXPECT_EQ(result1->count, 2);
    auto span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), result1->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer0.begin(), buffer0.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[1].mem), result1->buf[1].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer1.begin(), buffer1.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

TEST(LeafTest, LeafAddDifferentRangeDataTest)
{
    Leaf root = GetTestLeaf();

    Protocol::IPacket::List data;
    constexpr size_t PacketSize = 1400;
    std::array<std::byte, PacketSize> buffer0 {};
    for (auto& byte : buffer0) {
        byte = std::byte(0);
    }
    auto packet0 = std::make_unique<Protocol::Packet>(1500);
    packet0->SetPayload(buffer0);
    data.push_back(std::move(packet0));

    root.AddData(0, PacketSize, std::move(data));

    auto result1 = root.GetData(0, 1400);

    EXPECT_EQ(result1->count, 1);
    auto span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), result1->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer0.begin(), buffer0.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    Protocol::IPacket::List data2;
    std::array<std::byte, PacketSize> buffer1 {};
    for (auto& byte : buffer1) {
        byte = std::byte(1);
    }
    auto packet1 = std::make_unique<Protocol::Packet>(1500);
    packet1->SetPayload(buffer1);
    data2.push_back(std::move(packet1));

    root.AddData(1400, PacketSize, std::move(data2));

    result1 = root.GetData(0, 1400);

    EXPECT_EQ(result1->count, 1);
    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), result1->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer0.begin(), buffer0.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    result1 = root.GetData(1400, 1400);
    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), result1->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer1.begin(), buffer1.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

TEST(LeafTest, LeafAddDifferentRangeDataTest2)
{
    Leaf root = GetTestLeaf();

    Protocol::IPacket::List data;
    constexpr size_t PacketSize = 1400;

    Protocol::IPacket::List data2;
    std::array<std::byte, PacketSize> buffer1 {};
    for (auto& byte : buffer1) {
        byte = std::byte(1);
    }
    auto packet1 = std::make_unique<Protocol::Packet>(1500);
    packet1->SetPayload(buffer1);
    data2.push_back(std::move(packet1));

    root.AddData(1400, PacketSize, std::move(data2));

    std::array<std::byte, PacketSize> buffer0 {};
    for (auto& byte : buffer0) {
        byte = std::byte(0);
    }
    auto packet0 = std::make_unique<Protocol::Packet>(1500);
    packet0->SetPayload(buffer0);
    data.push_back(std::move(packet0));

    root.AddData(0, PacketSize, std::move(data));

    auto result1 = root.GetData(0, 1400);

    EXPECT_EQ(result1->count, 1);
    auto span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), result1->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer0.begin(), buffer0.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    result1 = root.GetData(1400, 1400);
    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), result1->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer1.begin(), buffer1.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                                                                    //
    result1 = root.GetData(0, 2800);
    EXPECT_EQ(result1->count, 2);

    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), result1->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer0.begin(), buffer0.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[1].mem), result1->buf[1].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer1.begin(), buffer1.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

TEST(LeafTest, LeafAddDifferentRangeDataTest3)
{
    Leaf root = GetTestLeaf();

    constexpr size_t PacketSize = 1400;

    std::array<std::byte, PacketSize> buffer2 {};
    for (auto& byte : buffer2) {
        byte = std::byte(2);
    }
    Protocol::IPacket::List data2;
    auto packet2 = std::make_unique<Protocol::Packet>(1500);
    packet2->SetPayload(buffer2);
    data2.push_back(std::move(packet2));

    root.AddData(2800, PacketSize, std::move(data2));

    std::array<std::byte, PacketSize> buffer0 {};
    for (auto& byte : buffer0) {
        byte = std::byte(0);
    }
    Protocol::IPacket::List data0;
    auto packet0 = std::make_unique<Protocol::Packet>(1500);
    packet0->SetPayload(buffer0);
    data0.push_back(std::move(packet0));

    root.AddData(0, PacketSize, std::move(data0));

    std::array<std::byte, PacketSize> buffer1 {};
    for (auto& byte : buffer1) {
        byte = std::byte(1);
    }
    Protocol::IPacket::List data1;
    auto packet1 = std::make_unique<Protocol::Packet>(1500);
    packet1->SetPayload(buffer1);
    data1.push_back(std::move(packet1));

    root.AddData(1400, PacketSize, std::move(data1));

    auto result1 = root.GetData(0, 1400);

    EXPECT_EQ(result1->count, 1);
    auto span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), result1->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer0.begin(), buffer0.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    result1 = root.GetData(1400, 1400);
    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), result1->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer1.begin(), buffer1.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                                                                    //
    result1 = root.GetData(0, 4200);
    EXPECT_EQ(result1->count, 3);

    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), result1->buf[0].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer0.begin(), buffer0.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[1].mem), result1->buf[1].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer1.begin(), buffer1.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[2].mem), result1->buf[2].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer2.begin(), buffer2.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

TEST(LeafTest, LeafAddIntersectionDataTest)
{
    return;
    Leaf root = GetTestLeaf();

    Protocol::IPacket::List data;
    constexpr size_t PacketSize = 1300;
    std::array<std::byte, PacketSize> buffer0 {};
    for (auto& byte : buffer0) {
        byte = std::byte(0);
    }
    auto packet0 = std::make_unique<Protocol::Packet>(1400);
    packet0->SetPayload(buffer0);
    data.push_back(std::move(packet0));

    root.AddData(0, PacketSize, std::move(data));

    Protocol::IPacket::List data2;
    std::array<std::byte, PacketSize> buffer1 {};
    for (auto& byte : buffer1) {
        byte = std::byte(1);
    }
    auto packet1 = std::make_unique<Protocol::Packet>(1400);
    packet1->SetPayload(buffer1);
    data.push_back(std::move(packet1));

    root.AddData(600, PacketSize, std::move(data));

    auto result1 = root.GetData(0, 2800);

    EXPECT_EQ(result1->count, 2);

    auto span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[0].mem), 600); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer0.begin(), buffer0.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    span = std::span<std::byte>(static_cast<std::byte*>(result1->buf[1].mem), result1->buf[1].size); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_THAT(span, ::testing::ElementsAreArray(buffer1.begin(), buffer1.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

TEST(LeafTest, LeafAddIntersectionDataTest2)
{
    Leaf root = GetTestLeaf();
    constexpr auto Offset1MB = static_cast<const size_t>(1024 * 1024);
    constexpr auto PacketSize = static_cast<const size_t>(1024 * 1024);

    constexpr char Value0 = 0;
    auto data = GetTestData(PacketSize, Value0);
    root.AddData(Offset1MB, PacketSize, std::move(data));

    constexpr char Value1 = 1;
    Protocol::IPacket::List data1 = GetTestData(PacketSize, Value1);
    root.AddData(2 * Offset1MB, PacketSize, std::move(data1));

    auto result1 = root.GetData(0, 2800);
    EXPECT_EQ(result1->count, 0);

    auto result2 = root.GetData(Offset1MB + 1, 2800);
    EXPECT_EQ(result2->count, 3);

    auto result3 = root.GetData(Offset1MB, 2800);
    EXPECT_EQ(result3->count, 2);

    result1 = root.GetData(0, 2800);
    EXPECT_EQ(result1->count, 0);

    result2 = root.GetData(Offset1MB + 1, 2800);
    EXPECT_EQ(result2->count, 3);

    result3 = root.GetData(Offset1MB, 2800);
    EXPECT_EQ(result3->count, 2);
    char* buf = static_cast<char*>(result3->buf[0].mem);
    EXPECT_EQ(buf[0], Value0); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_EQ(buf[1], Value0); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    auto result4 = root.GetData(2 * Offset1MB, 2800);
    buf = static_cast<char*>(result4->buf[0].mem);
    EXPECT_EQ(buf[0], Value1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_EQ(buf[1], Value1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

TEST(LeafTest, LeafAddIntersectionDataTest3)
{
    Leaf root = GetTestLeaf();
    constexpr auto Offset512KB = static_cast<const size_t>(512 * 1024);
    constexpr auto PacketSize = static_cast<const size_t>(512 * 1024);

    constexpr char Value0 = 0;
    auto data = GetTestData(PacketSize, Value0);
    root.AddData(0, PacketSize, std::move(data));

    constexpr char Value1 = 1;
    Protocol::IPacket::List data1 = GetTestData(PacketSize, Value1);
    root.AddData(Offset512KB, PacketSize, std::move(data1));

    auto result1 = root.GetData(0, 2 * PacketSize);
    size_t size = 0;
    for (int index = 0; index < result1->count; index++) {
        size += result1->buf[index].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    EXPECT_EQ(size, PacketSize * 2);
}

TEST(LeafTest, LeafAddIntersectionDataTest4)
{
    Leaf root = GetTestLeaf();
    constexpr auto Offset512KB = static_cast<const size_t>(512 * 1024);
    constexpr auto PacketSize = static_cast<const size_t>(512 * 1024);

    constexpr char Value0 = 0;
    auto data = GetTestData(PacketSize, Value0);
    root.AddData(Offset512KB, PacketSize, std::move(data));

    constexpr char Value1 = 1;
    Protocol::IPacket::List data1 = GetTestData(PacketSize * 2, Value1);
    auto freePackets = root.AddData(0, PacketSize * 2, std::move(data1));

    auto result1 = root.GetData(0, 2 * PacketSize);
    size_t size = 0;
    for (int index = 0; index < result1->count; index++) {
        size += result1->buf[index].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    EXPECT_EQ(size, PacketSize * 2);

    char* buf = static_cast<char*>(result1->buf[0].mem);
    EXPECT_EQ(buf[0], Value1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_EQ(buf[1], Value1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                               //
    buf = static_cast<char*>(result1->buf[result1->count - 1].mem); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    EXPECT_EQ(buf[0], Value1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    size = 0;
    for (auto& packet : freePackets) {
        size += packet->GetPayload().size();
    }

    EXPECT_EQ(size, PacketSize);
}

TEST(LeafTest, LeafGetData2MBTest)
{
    Leaf root = GetTestLeaf();
    constexpr auto Offset1MB = static_cast<const size_t>(1024 * 1024);
    constexpr auto PacketSize = static_cast<const size_t>(1024 * 1024);

    constexpr char Value1 = 1;
    auto data = GetTestData(PacketSize, Value1);
    root.AddData(Offset1MB, PacketSize, std::move(data));

    auto result1 = root.GetData(Offset1MB, 2 * PacketSize);
    size_t size = 0;
    for (int index = 0; index < result1->count; index++) {
        size += result1->buf[index].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    EXPECT_EQ(size, PacketSize);

    constexpr char Value0 = 0;
    Protocol::IPacket::List data1 = GetTestData(PacketSize, Value0);
    auto freePackets = root.AddData(0, PacketSize, std::move(data1));

    auto result2 = root.GetData(0, 2 * PacketSize);
    size = 0;
    for (int index = 0; index < result2->count; index++) {
        size += result2->buf[index].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    EXPECT_EQ(size, PacketSize * 2);

    char* buf = static_cast<char*>(result2->buf[0].mem);
    EXPECT_EQ(buf[0], Value0); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_EQ(buf[1], Value0); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                               //
    buf = static_cast<char*>(result2->buf[result2->count - 1].mem); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    EXPECT_EQ(buf[0], Value1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    EXPECT_EQ(buf[1], Value1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    EXPECT_EQ(freePackets.size(), 0);

    auto result3 = root.GetData(Offset1MB - 1, 2 * PacketSize);
    size = 0;
    for (int index = 0; index < result3->count; index++) {
        size += result3->buf[index].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    EXPECT_EQ(size, PacketSize + 1);

    buf = static_cast<char*>(result3->buf[0].mem);
    EXPECT_EQ(buf[0], Value0); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                               //
    buf = static_cast<char*>(result3->buf[1].mem); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    EXPECT_EQ(buf[0], Value1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                               //
    auto result4 = root.GetData(Offset1MB + 1, 2 * PacketSize);
    size = 0;
    for (int index = 0; index < result4->count; index++) {
        size += result4->buf[index].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    EXPECT_EQ(size, PacketSize - 1);

    buf = static_cast<char*>(result4->buf[0].mem);
    EXPECT_EQ(buf[0], Value1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                               //
    buf = static_cast<char*>(result4->buf[1].mem); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    EXPECT_EQ(buf[0], Value1); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

} // namespace FastTransport::FileSystem
