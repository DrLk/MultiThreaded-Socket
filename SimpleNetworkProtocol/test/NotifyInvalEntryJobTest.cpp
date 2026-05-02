#include <gtest/gtest.h>

#ifdef __linux__

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <stop_token>

#include "FileSystem/NotifyInvalEntryJob.hpp"
#include "FileTree.hpp"
#include "ITaskScheduler.hpp"
#include "InotifyWatcher.hpp"
#include "Leaf.hpp"
#include "MessageWriter.hpp"
#include "Packet.hpp"

namespace {

using FastTransport::FileSystem::FileTree;
using FastTransport::FileSystem::Leaf;
using FastTransport::FileSystem::WatchEventType;
using FastTransport::Protocol::IPacket;
using FastTransport::Protocol::MessageWriter;
using FastTransport::Protocol::Packet;
using FastTransport::TaskQueue::ITaskScheduler;
using FastTransport::TaskQueue::NotifyInvalEntryJob;

// NOLINTBEGIN(cppcoreguidelines-rvalue-reference-param-not-moved)
class StubScheduler : public ITaskScheduler {
public:
    void Schedule(std::unique_ptr<FastTransport::TaskQueue::Job>&& /*job*/) override { }
    void Wait(std::stop_token /*stop*/) override { }
    void ScheduleMainJob(std::unique_ptr<FastTransport::TaskQueue::MainJob>&& /*job*/) override { }
    void ScheduleMainReadJob(std::unique_ptr<FastTransport::TaskQueue::MainReadJob>&& /*job*/) override { }
    void ScheduleWriteNetworkJob(std::unique_ptr<FastTransport::TaskQueue::WriteNetworkJob>&& /*job*/) override { }
    void ScheduleReadNetworkJob(std::unique_ptr<FastTransport::TaskQueue::ReadNetworkJob>&& /*job*/) override { }
    void ScheduleDiskJob(std::unique_ptr<FastTransport::TaskQueue::DiskJob>&& /*job*/) override { }
    void ScheduleFuseNetworkJob(std::unique_ptr<FastTransport::TaskQueue::FuseNetworkJob>&& /*job*/) override { }
    void ScheduleResponseFuseNetworkJob(std::unique_ptr<FastTransport::TaskQueue::ResponseFuseNetworkJob>&& /*job*/) override { }
    void ScheduleResponseReadDiskJob(std::unique_ptr<FastTransport::TaskQueue::ResponseReadFileJob>&& /*job*/) override { }
    void ScheduleResponseInFuseNetworkJob(std::unique_ptr<FastTransport::TaskQueue::ResponseInFuseNetworkJob>&& /*job*/) override { }
    void ScheduleCacheTreeJob(std::unique_ptr<FastTransport::TaskQueue::CacheTreeJob>&& /*job*/) override { }
    void ScheduleInotifyWatcherJob(std::unique_ptr<FastTransport::TaskQueue::InotifyWatcherJob>&& /*job*/) override { }
    void ReturnFreeDiskPackets(IPacket::List&& /*packets*/) override { }
};
// NOLINTEND(cppcoreguidelines-rvalue-reference-param-not-moved)

IPacket::List MakeFreePackets(int count)
{
    IPacket::List packets;
    for (int i = 0; i < count; i++) {
        auto packet = std::make_unique<Packet>(1500);
        std::array<std::byte, 1000> payload {};
        packet->SetPayload(payload);
        packets.push_back(std::move(packet));
    }
    return packets;
}

FastTransport::Protocol::MessageReader EncodeEntry(WatchEventType eventType, std::uint64_t parentServerInode, const std::string& name, std::uint64_t serverInode, std::uint64_t size, bool isDir)
{
    constexpr int PacketCount = 4;
    MessageWriter writer(MakeFreePackets(PacketCount));
    writer << static_cast<std::uint32_t>(eventType);
    writer << parentServerInode;
    writer << name;
    writer << serverInode;
    writer << size;
    writer << static_cast<std::uint8_t>(isDir ? 1 : 0);
    return FastTransport::Protocol::MessageReader(writer.GetWritedPackets());
}

TEST(NotifyInvalEntryJobTest, CreateAddsChildToFileTree)
{
    FileTree tree("/tmp/test", "/tmp/test/cache");
    Leaf& root = tree.GetRoot();
    const Leaf& parentDir = root.AddChild("parentDir", std::filesystem::file_type::directory, 0);
    const std::uint64_t parentServerInode = parentDir.GetServerInode();
    constexpr std::uint64_t NewServerInode = 12345;

    auto job = std::make_unique<NotifyInvalEntryJob>();
    job->InitReader(EncodeEntry(WatchEventType::Created, parentServerInode, "newfile", NewServerInode, 100, false));

    StubScheduler scheduler;
    const std::stop_source stopSource;
    job->ExecuteResponse(scheduler, stopSource.get_token(), tree);

    const auto& children = parentDir.GetChildren();
    EXPECT_EQ(children.count("newfile"), 1U);
    const Leaf& newLeaf = children.at("newfile");
    EXPECT_EQ(newLeaf.GetType(), std::filesystem::file_type::regular);
    EXPECT_EQ(newLeaf.GetSize(), 100U);
    EXPECT_EQ(newLeaf.GetServerInode(), NewServerInode);
}

TEST(NotifyInvalEntryJobTest, DeleteRemovesChildFromFileTree)
{
    FileTree tree("/tmp/test", "/tmp/test/cache");
    Leaf& root = tree.GetRoot();
    Leaf& parentDir = root.AddChild("parentDir", std::filesystem::file_type::directory, 0);
    parentDir.AddChild("existingfile", std::filesystem::file_type::regular, 200);
    const std::uint64_t parentServerInode = parentDir.GetServerInode();

    auto job = std::make_unique<NotifyInvalEntryJob>();
    job->InitReader(EncodeEntry(WatchEventType::Deleted, parentServerInode, "existingfile", 0, 0, false));

    StubScheduler scheduler;
    const std::stop_source stopSource;
    job->ExecuteResponse(scheduler, stopSource.get_token(), tree);

    EXPECT_EQ(parentDir.GetChildren().count("existingfile"), 0U);
}

TEST(NotifyInvalEntryJobTest, CreateDirectoryAddsDirectoryLeaf)
{
    FileTree tree("/tmp/test", "/tmp/test/cache");
    Leaf& root = tree.GetRoot();
    const Leaf& parentDir = root.AddChild("parentDir", std::filesystem::file_type::directory, 0);
    const std::uint64_t parentServerInode = parentDir.GetServerInode();
    constexpr std::uint64_t NewDirServerInode = 99999;

    auto job = std::make_unique<NotifyInvalEntryJob>();
    job->InitReader(EncodeEntry(WatchEventType::Created, parentServerInode, "newdir", NewDirServerInode, 0, true));

    StubScheduler scheduler;
    const std::stop_source stopSource;
    job->ExecuteResponse(scheduler, stopSource.get_token(), tree);

    const auto& children = parentDir.GetChildren();
    EXPECT_EQ(children.count("newdir"), 1U);
    EXPECT_EQ(children.at("newdir").GetType(), std::filesystem::file_type::directory);
    EXPECT_EQ(children.at("newdir").GetServerInode(), NewDirServerInode);
}

} // namespace

#endif // __linux__
