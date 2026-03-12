#include <gtest/gtest.h>

#ifdef __linux__

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
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
    constexpr size_t ChunkSize = 1024UL * 1024UL;
    std::vector<char> chunk(ChunkSize);
    for (size_t i = 0; i < TestFileSize / ChunkSize; ++i) {
        for (size_t j = 0; j < ChunkSize; ++j) {
            chunk[j] = static_cast<char>((i * ChunkSize + j) % 256);
        }
        file.write(chunk.data(), static_cast<std::streamsize>(ChunkSize));
    }
}

void PrepareDirectories()
{
    std::filesystem::create_directories(MountPoint);
    std::filesystem::create_directories(CacheDir);
}

void Unmount()
{
    const std::string cmd = std::string("fusermount3 -u ") + MountPoint + " 2>/dev/null";
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

    // Server thread: serves files from /tmp, accepts incoming connections
    std::jthread serverThread([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", ServerPort));

        const IConnection::Ptr srcConnection = src.Accept(stop);
        if (srcConnection == nullptr) {
            return;
        }

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

    // Client thread: connects to server and mounts FUSE at MountPoint
    std::jthread clientThread([](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", ClientPort));
        const ConnectionAddr serverAddr("127.0.0.1", ServerPort);

        const IConnection::Ptr dstConnection = dst.Connect(serverAddr);

        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);
        IPacket::List sendPackets = UDPQueue::CreateBuffers(200000);
        dstConnection->AddFreeRecvPackets(std::move(recvPackets));
        dstConnection->AddFreeSendPackets(std::move(sendPackets));

        FileTree fileTree("/tmp/rfs_client_tree", CacheDir);
        // filesystem declared before scheduler so that ~scheduler (joins worker
        // threads, completing all pending fuse_reply_* calls) runs before
        // ~filesystem (calls fuse_session_destroy).
        RemoteFileSystem filesystem(MountPoint);
        TaskScheduler scheduler(*dstConnection, fileTree);
        scheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));

        RemoteFileSystem::scheduler = &scheduler;
        filesystem.Start(stop); // blocks until unmounted or stop requested

        scheduler.Wait(stop);
    });

    // Read thread: waits for mount, reads via FUSE and verifies
    std::jthread readThread([&testResult, &stopSource](std::stop_token /*stop*/) {
        std::this_thread::sleep_for(5s);

        std::ifstream src(TestFilePath, std::ios::binary);
        const std::string mntFile = std::string(MountPoint) + "/" + TestFileName;
        std::ifstream mnt(mntFile, std::ios::binary);

        if (!src || !mnt) {
            stopSource.request_stop();
            Unmount();
            return;
        }

        constexpr size_t ChunkSize = 1024UL * 1024UL;
        std::vector<char> srcBuf(ChunkSize);
        std::vector<char> mntBuf(ChunkSize);
        size_t totalRead = 0;
        bool match = true;

        while (totalRead < TestFileSize) {
            const size_t remaining = TestFileSize - totalRead;
            const auto toRead = static_cast<std::streamsize>(std::min(ChunkSize, remaining));

            if (!src.read(srcBuf.data(), toRead) || src.gcount() != toRead) {
                match = false;
                break;
            }
            if (!mnt.read(mntBuf.data(), toRead) || mnt.gcount() != toRead) {
                match = false;
                break;
            }

            if (std::memcmp(srcBuf.data(), mntBuf.data(), static_cast<size_t>(toRead)) != 0) {
                match = false;
                break;
            }

            totalRead += static_cast<size_t>(toRead);
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
    readThread.join();

    EXPECT_TRUE(testResult) << "File read via FUSE did not match original ("
                            << TestFileSize << " bytes expected)";
}

#endif // __linux__
