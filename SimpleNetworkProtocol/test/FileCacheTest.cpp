#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>

#include "FileTree.hpp"
#include "Leaf.hpp"
#include "Packet.hpp"
#include "PieceStatus.hpp"

namespace {

FastTransport::Protocol::IPacket::List MakePackets(size_t count, char value)
{
    FastTransport::Protocol::IPacket::List data;
    std::array<std::byte, 1400> buffer {};
    for (auto& byte : buffer) {
        byte = std::byte(value);
    }
    for (size_t i = 0; i < count; i++) {
        auto packet = std::make_unique<FastTransport::Protocol::Packet>(1500);
        packet->SetPayload(buffer);
        data.push_back(std::move(packet));
    }
    return data;
}

} // namespace

namespace FastTransport::FileSystem {

// ---------------------------------------------------------------------------
// Leaf::GetFirstBlockIndex tests
// ---------------------------------------------------------------------------

TEST(LeafCacheTest, GetFirstBlockIndexEmptyLeaf)
{
    Leaf root("test", std::filesystem::file_type::regular, 1400, nullptr);
    EXPECT_EQ(root.GetFirstBlockIndex(), SIZE_MAX);
}

TEST(LeafCacheTest, GetFirstBlockIndexSingleBlock)
{
    Leaf root("test", std::filesystem::file_type::regular, 1400, nullptr);
    auto data = MakePackets(1, 0);
    root.AddData(0, 1400, std::move(data));
    EXPECT_EQ(root.GetFirstBlockIndex(), 0);
}

TEST(LeafCacheTest, GetFirstBlockIndexReturnsMinimum)
{
    // Add block 2 first, then block 0 — should always return 0 (minimum)
    Leaf root("test", std::filesystem::file_type::regular, 3 * Leaf::BlockSize, nullptr);

    auto data2 = MakePackets(1, 0);
    root.AddData(2 * Leaf::BlockSize, 1400, std::move(data2));

    auto data0 = MakePackets(1, 0);
    root.AddData(0, 1400, std::move(data0));

    EXPECT_EQ(root.GetFirstBlockIndex(), 0);
}

// ---------------------------------------------------------------------------
// FileTree cache eviction tests
// ---------------------------------------------------------------------------

TEST(FileTreeCacheTest, NeedsEvictionFalseInitially)
{
    FileTree tree("/tmp/test_cache_tree", "/tmp/test_cache_tree/cache");
    EXPECT_FALSE(tree.NeedsEviction());
}

TEST(FileTreeCacheTest, NeedsEvictionFalseUnderLimit)
{
    FileTree tree("/tmp/test_cache_tree", "/tmp/test_cache_tree/cache");
    Leaf& file = tree.GetRoot().AddChild("file", std::filesystem::file_type::regular, 100LL * 1024 * 1024);
    auto inode = reinterpret_cast<fuse_ino_t>(&file); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

    // 100 packets << MaxCachePackets (10000)
    auto data = MakePackets(100, 0);
    tree.AddData(inode, 0, 100 * 1400UL, std::move(data));

    EXPECT_FALSE(tree.NeedsEviction());
}

TEST(FileTreeCacheTest, NeedsEvictionTrueOverLimit)
{
    FileTree tree("/tmp/test_cache_tree", "/tmp/test_cache_tree/cache");
    Leaf& file = tree.GetRoot().AddChild("file", std::filesystem::file_type::regular, 20LL * Leaf::BlockSize);
    auto inode = reinterpret_cast<fuse_ino_t>(&file); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

    // 12 blocks × 900 packets = 10800 > MaxCachePackets (10000)
    constexpr size_t PacketsPerBlock = 900;
    constexpr size_t BlockDataSize = PacketsPerBlock * 1400;
    for (int block = 0; block < 12; block++) {
        auto data = MakePackets(PacketsPerBlock, 0);
        tree.AddData(inode, static_cast<off_t>(block) * Leaf::BlockSize, BlockDataSize, std::move(data));
    }

    EXPECT_TRUE(tree.NeedsEviction());
}

TEST(FileTreeCacheTest, GetFreeDataEmptyWhenNoCachedData)
{
    FileTree tree("/tmp/test_cache_tree", "/tmp/test_cache_tree/cache");
    auto freeData = tree.GetFreeData();
    EXPECT_EQ(freeData.data.size(), 0);
}

TEST(FileTreeCacheTest, GetFreeDataDecrementsUntilUnderLimit)
{
    FileTree tree("/tmp/test_cache_tree", "/tmp/test_cache_tree/cache");
    Leaf& file = tree.GetRoot().AddChild("file", std::filesystem::file_type::regular, 20LL * Leaf::BlockSize);
    auto inode = reinterpret_cast<fuse_ino_t>(&file); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

    constexpr size_t PacketsPerBlock = 900;
    constexpr size_t BlockDataSize = PacketsPerBlock * 1400;
    for (int block = 0; block < 12; block++) {
        auto data = MakePackets(PacketsPerBlock, 0);
        tree.AddData(inode, static_cast<off_t>(block) * Leaf::BlockSize, BlockDataSize, std::move(data));
    }
    ASSERT_TRUE(tree.NeedsEviction());

    while (tree.NeedsEviction()) {
        auto freeData = tree.GetFreeData();
        if (freeData.data.size() == 0) {
            break;
        }
    }

    EXPECT_FALSE(tree.NeedsEviction());
}

TEST(FileTreeCacheTest, GetFreeDataSetsStatusNotFound)
{
    FileTree tree("/tmp/test_cache_tree", "/tmp/test_cache_tree/cache");
    Leaf& file = tree.GetRoot().AddChild("file", std::filesystem::file_type::regular, 100LL * 1024 * 1024);
    auto inode = reinterpret_cast<fuse_ino_t>(&file); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

    auto data = MakePackets(1, 0);
    tree.AddData(inode, 0, 1400, std::move(data));

    auto piecesStatus = file.GetPiecesStatus();
    EXPECT_EQ(piecesStatus->GetStatus(0), PieceStatus::InCache);

    tree.GetFreeData();

    EXPECT_EQ(piecesStatus->GetStatus(0), PieceStatus::NotFound);
}

TEST(FileTreeCacheTest, GetFreeDataSetsCorrectSizeForFullBlock)
{
    FileTree tree("/tmp/test_cache_tree", "/tmp/test_cache_tree/cache");
    Leaf& file = tree.GetRoot().AddChild("file", std::filesystem::file_type::regular, static_cast<uintmax_t>(3 * Leaf::BlockSize));
    auto inode = reinterpret_cast<fuse_ino_t>(&file); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

    auto data = MakePackets(100, 0);
    tree.AddData(inode, 0, 100 * 1400, std::move(data));

    auto freeData = tree.GetFreeData();
    ASSERT_GT(freeData.data.size(), 0);
    EXPECT_EQ(freeData.size, static_cast<size_t>(Leaf::BlockSize));
}

TEST(FileTreeCacheTest, GetFreeDataSetsCorrectSizeForLastBlock)
{
    constexpr size_t LastBlockBytes = 1400;
    FileTree tree("/tmp/test_cache_tree", "/tmp/test_cache_tree/cache");
    Leaf& file = tree.GetRoot().AddChild("file", std::filesystem::file_type::regular,
        static_cast<uintmax_t>(Leaf::BlockSize) + LastBlockBytes);
    auto inode = reinterpret_cast<fuse_ino_t>(&file); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

    // Add one packet to block 1 (last/partial block)
    auto data = MakePackets(1, 0);
    const off_t lastBlockOffset = Leaf::BlockSize;
    tree.AddData(inode, lastBlockOffset, LastBlockBytes, std::move(data));

    auto freeData = tree.GetFreeData();
    ASSERT_GT(freeData.data.size(), 0);
    EXPECT_EQ(freeData.size, LastBlockBytes);
}

TEST(FileTreeCacheTest, GetFreeDataEvictsOldestBlock)
{
    FileTree tree("/tmp/test_cache_tree", "/tmp/test_cache_tree/cache");
    Leaf& file = tree.GetRoot().AddChild("file", std::filesystem::file_type::regular, 20LL * Leaf::BlockSize);
    auto inode = reinterpret_cast<fuse_ino_t>(&file); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

    // Add data to blocks 0 and 2
    auto data0 = MakePackets(1, 0);
    tree.AddData(inode, 0, 1400, std::move(data0));

    auto data2 = MakePackets(1, 0);
    tree.AddData(inode, 2 * Leaf::BlockSize, 1400, std::move(data2));

    // GetFreeData should evict block 0 (smallest index)
    auto freeData = tree.GetFreeData();
    ASSERT_GT(freeData.data.size(), 0);
    EXPECT_EQ(freeData.offset, 0);

    // Block 0 should now be NotFound, block 2 still InCache
    auto piecesStatus = file.GetPiecesStatus();
    EXPECT_EQ(piecesStatus->GetStatus(0), PieceStatus::NotFound);
    EXPECT_EQ(piecesStatus->GetStatus(2), PieceStatus::InCache);
}

} // namespace FastTransport::FileSystem
