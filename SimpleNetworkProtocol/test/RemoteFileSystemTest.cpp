#include <gtest/gtest.h>

#ifdef __linux__

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stop_token>
#include <thread>
#include <vector>

#include "ConnectionAddr.hpp"
#include "FastTransportProtocol.hpp"
#include "FileSystem/RemoteFileSystem.hpp"
#include "IPacket.hpp"
#include "MessageTypeReadJob.hpp"
#include "TaskScheduler.hpp"
#include "UDPQueue.hpp"

using namespace std::chrono_literals;
using FastTransport::FileSystem::FileTree;
using FastTransport::Protocol::ConnectionAddr;
using FastTransport::Protocol::FastTransportContext;
using FastTransport::Protocol::IConnection;
using FastTransport::Protocol::IPacket;
using FastTransport::Protocol::UDPQueue;
using FastTransport::TaskQueue::MessageTypeReadJob;
using FastTransport::TaskQueue::RemoteFileSystem;
using FastTransport::TaskQueue::TaskScheduler;

namespace {

constexpr uint16_t ServerPort = 31100;
constexpr uint16_t ClientPort = 31200;
constexpr size_t TestFileSize = 10ULL * 1024 * 1024; // 10 MB → 7+ blocks
constexpr const char* TestFilePath = "/tmp/rfs_test_file.bin";
constexpr const char* TestFileName = "rfs_test_file.bin";
constexpr const char* MountPoint = "/tmp/mnt_rfs_test";
constexpr const char* CacheDir = "/tmp/rfs_test_cache";

void CreateTestFile()
{
    std::ofstream file(TestFilePath, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(file) << "Failed to create test file: " << TestFilePath;
    const std::vector<char> chunk(1024UL * 1024UL, 0x42);
    for (size_t i = 0; i < TestFileSize / chunk.size(); ++i) {
        file.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
    }
}

void PrepareDirectories()
{
    std::filesystem::create_directories(MountPoint);
    std::filesystem::create_directories(CacheDir);
}

void Unmount()
{
    std::string cmd = std::string("fusermount3 -u ") + MountPoint + " 2>/dev/null";
    std::system(cmd.c_str()); // NOLINT(cert-env33-c, concurrency-mt-unsafe)
}

} // namespace

TEST(RemoteFileSystemTest, ReadFileOverNetwork) // NOLINT(readability-function-cognitive-complexity)
{
    PrepareDirectories();
    Unmount(); // clean up any leftover mount from previous run
    CreateTestFile();
    std::filesystem::remove_all(CacheDir);
    std::filesystem::create_directories(CacheDir);

    std::atomic<bool> testResult { false };
    std::stop_source stopSource;

    // Server thread: serves files from /tmp
    std::jthread serverThread([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", ServerPort));
        const ConnectionAddr dstAddr("127.0.0.1", ClientPort);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);
        IPacket::List sendPackets = UDPQueue::CreateBuffers(200000);
        srcConnection->AddFreeRecvPackets(std::move(recvPackets));
        srcConnection->AddFreeSendPackets(std::move(sendPackets));

        FileTree fileTree("/tmp", CacheDir);
        TaskScheduler scheduler(*srcConnection, fileTree);
        scheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));
        scheduler.Wait(stop);
    });

    std::this_thread::sleep_for(500ms);

    // Client thread: accepts, mounts FUSE at MountPoint
    std::jthread clientThread([](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", ClientPort));

        const IConnection::Ptr dstConnection = dst.Accept(stop);
        if (dstConnection == nullptr) {
            return;
        }

        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);
        IPacket::List sendPackets = UDPQueue::CreateBuffers(200000);
        dstConnection->AddFreeRecvPackets(std::move(recvPackets));
        dstConnection->AddFreeSendPackets(std::move(sendPackets));

        FileTree fileTree("/tmp/rfs_client_tree", CacheDir);
        TaskScheduler scheduler(*dstConnection, fileTree);
        scheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));

        RemoteFileSystem filesystem(MountPoint);
        RemoteFileSystem::scheduler = &scheduler;
        filesystem.Start(); // blocks until unmounted

        scheduler.Wait(stop);
    });

    // Read thread: waits for mount, reads via FUSE and verifies
    std::jthread readThread([&testResult, &stopSource](std::stop_token /*stop*/) {
        std::this_thread::sleep_for(5s);

        std::ifstream src(TestFilePath, std::ios::binary);
        std::string mntFile = std::string(MountPoint) + "/" + TestFileName;
        std::ifstream mnt(mntFile, std::ios::binary);

        if (!src || !mnt) {
            stopSource.request_stop();
            Unmount();
            return;
        }

        std::vector<char> srcBuf(1024 * 1024);
        std::vector<char> mntBuf(1024UL * 1024UL);
        size_t totalRead = 0;
        bool match = true;

        while (src.read(srcBuf.data(), static_cast<std::streamsize>(srcBuf.size()))
            && mnt.read(mntBuf.data(), static_cast<std::streamsize>(mntBuf.size()))) {
            totalRead += static_cast<size_t>(src.gcount());
            if (srcBuf != mntBuf) {
                match = false;
                break;
            }
        }

        testResult = match && totalRead == TestFileSize;
        stopSource.request_stop();
        Unmount();
    });

    // Wait for readThread to signal done (max 30s)
    auto deadline = std::chrono::steady_clock::now() + 30s;
    while (!stopSource.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }

    if (!stopSource.stop_requested()) {
        stopSource.request_stop();
        Unmount();
    }

    serverThread.request_stop();
    clientThread.request_stop();

    EXPECT_TRUE(testResult) << "File read via FUSE did not match original ("
                            << TestFileSize << " bytes expected)";
}

#endif // __linux__
