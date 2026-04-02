#include <gtest/gtest.h>

#ifdef __linux__

#include <atomic>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <stop_token>
#include <dirent.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#include "ConnectionAddr.hpp"
#include "FastTransportProtocol.hpp"
#include "FileSystem/RemoteFileSystem.hpp"
#include "IPacket.hpp"
#include "Logger.hpp"
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
constexpr size_t TestFileSize = 100ULL * 1024 * 1024; // 100 MB
constexpr const char* TestFilePath = "/tmp/rfs_test_file.bin";
constexpr const char* TestFileName = "rfs_test_file.bin";
constexpr const char* MountPoint = "/tmp/mnt_rfs_test";
constexpr const char* CacheDir = "/tmp/rfs_test_cache";
constexpr const char* CopyFilePath = "/tmp/rfs_test_copy.bin";
constexpr const char* Copy2FilePath = "/tmp/rfs_test_copy2.bin";

constexpr uint16_t ServerPort2 = 31300;
constexpr uint16_t ClientPort2 = 31400;
constexpr const char* MountPoint2 = "/tmp/mnt_rfs_test2";
constexpr const char* CacheDir2 = "/tmp/rfs_test_cache2";

constexpr uint16_t ServerPort3 = 31500;
constexpr uint16_t ClientPort3 = 31600;
constexpr const char* MountPoint3 = "/tmp/mnt_rfs_test3";
constexpr const char* CacheDir3 = "/tmp/rfs_test_cache3";

// Directory structure for multi-thread test:
// /tmp/rfs_test_tree/
//   ├── subdir_a/
//   │   └── file_a.bin   (30 MB)
//   ├── subdir_b/
//   │   └── file_b.bin   (20 MB)
//   └── file_root.bin    (10 MB)
constexpr const char* TestTreeRoot = "/tmp/rfs_test_tree";
constexpr size_t FileASize = 30ULL * 1024 * 1024;
constexpr size_t FileBSize = 30ULL * 1024 * 1024;
constexpr size_t FileRootSize = 30ULL * 1024 * 1024;

// Block size must match Leaf::BlockSize (1000 * 1300)
constexpr size_t FuseBlockSize = 1000UL * 1300UL;

void CreateTestFile()
{
    std::ofstream file(TestFilePath, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(file) << "Failed to create test file: " << TestFilePath;
    constexpr size_t ChunkSize = 1024UL * 1024UL;
    std::vector<char> chunk(ChunkSize);
    for (size_t i = 0; i < TestFileSize / ChunkSize; ++i) {
        for (size_t j = 0; j < ChunkSize; ++j) {
            chunk[j] = static_cast<char>(((i * ChunkSize) + j) % 256);
        }
        file.write(chunk.data(), static_cast<std::streamsize>(ChunkSize));
    }
}

void CreateTestFileAt(const std::filesystem::path& path, size_t size, char seed)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(file) << "Failed to create test file: " << path;
    constexpr size_t ChunkSize = 1024UL * 1024UL;
    std::vector<char> chunk(ChunkSize);
    for (size_t written = 0; written < size; written += ChunkSize) {
        const size_t toWrite = std::min(ChunkSize, size - written);
        for (size_t j = 0; j < toWrite; ++j) {
            chunk[j] = static_cast<char>((written + j + static_cast<size_t>(seed)) % 256);
        }
        file.write(chunk.data(), static_cast<std::streamsize>(toWrite));
    }
}

void CreateTestTree()
{
    std::filesystem::remove_all(TestTreeRoot);
    std::filesystem::create_directories(TestTreeRoot);
    CreateTestFileAt(std::filesystem::path(TestTreeRoot) / "subdir_a" / "file_a.bin", FileASize, 0);
    CreateTestFileAt(std::filesystem::path(TestTreeRoot) / "subdir_b" / "file_b.bin", FileBSize, 42);
    CreateTestFileAt(std::filesystem::path(TestTreeRoot) / "file_root.bin", FileRootSize, 99);
}

void Unmount3()
{
    const pid_t pid = fork();
    if (pid == 0) {
        std::string prog = "fusermount3";
        std::string arg1 = "-u";
        std::string arg2 = MountPoint3;
        std::array<char*, 4> args = { prog.data(), arg1.data(), arg2.data(), nullptr };
        execvp(args[0], args.data());
        _exit(1);
    }
    if (pid > 0) {
        waitpid(pid, nullptr, 0);
    }
}

void PrepareDirectories()
{
    std::filesystem::create_directories(MountPoint);
    std::filesystem::create_directories(CacheDir);
}

void Unmount()
{
    const pid_t pid = fork();
    if (pid == 0) {
        std::string prog = "fusermount3";
        std::string arg1 = "-u";
        std::string arg2 = MountPoint;
        std::array<char*, 4> args = { prog.data(), arg1.data(), arg2.data(), nullptr };
        execvp(args[0], args.data());
        _exit(1);
    }
    if (pid > 0) {
        waitpid(pid, nullptr, 0);
    }
}

bool CompareFiles(const std::string& path1, const std::string& path2, size_t totalSize)
{
    constexpr size_t ChunkSize = 1024UL * 1024UL;
    std::ifstream file1(path1, std::ios::binary);
    std::ifstream file2(path2, std::ios::binary);
    if (!file1 || !file2) {
        return false;
    }
    std::vector<char> buf1(ChunkSize);
    std::vector<char> buf2(ChunkSize);
    size_t totalRead = 0;
    while (totalRead < totalSize) {
        const size_t remaining = totalSize - totalRead;
        const auto toRead = static_cast<std::streamsize>(std::min(ChunkSize, remaining));
        if (!file1.read(buf1.data(), toRead) || file1.gcount() != toRead) {
            return false;
        }
        if (!file2.read(buf2.data(), toRead) || file2.gcount() != toRead) {
            return false;
        }
        if (std::memcmp(buf1.data(), buf2.data(), static_cast<size_t>(toRead)) != 0) {
            return false;
        }
        totalRead += static_cast<size_t>(toRead);
    }
    return totalRead == totalSize;
}

bool CopyFileViaMount(const std::string& src, const std::string& dst, const char* label)
{
    std::error_code error;
    std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, error);
    if (error) {
        std::cerr << "[test] " << label << " failed: " << error.message() << "\n";
        return false;
    }
    std::cerr << "[test] " << label << " succeeded, size=" << std::filesystem::file_size(dst) << "\n";
    return true;
}

bool RandomReadComparison(const std::string& original, const std::string& mounted, size_t fileSize)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg,android-cloexec-open)
    const int origFd = open(original.c_str(), O_RDONLY | O_CLOEXEC);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg,android-cloexec-open)
    const int mntFd = open(mounted.c_str(), O_RDONLY | O_CLOEXEC);
    if (origFd < 0 || mntFd < 0) {
        if (origFd >= 0) {
            close(origFd);
        }
        if (mntFd >= 0) {
            close(mntFd);
        }
        return false;
    }

    // Max read size: 3 blocks, so reads regularly cross block boundaries
    constexpr size_t MaxReadSize = 3 * FuseBlockSize;
    std::vector<char> origBuf(MaxReadSize);
    std::vector<char> mntBuf(MaxReadSize);

    std::mt19937_64 rng(std::random_device {}()); // NOLINT(cert-msc51-cpp)
    std::uniform_int_distribution<size_t> offsetDist(0, fileSize - 1);

    constexpr size_t NumReads = 50;
    for (size_t i = 0; i < NumReads; ++i) {
        const size_t offset = offsetDist(rng);
        const size_t maxSize = std::min(MaxReadSize, fileSize - offset);
        std::uniform_int_distribution<size_t> sizeDist(1, maxSize);
        const size_t size = sizeDist(rng);

        const ssize_t origRead = pread(origFd, origBuf.data(), size, static_cast<off_t>(offset));
        const ssize_t mntRead = pread(mntFd, mntBuf.data(), size, static_cast<off_t>(offset));

        if (std::cmp_not_equal(origRead, size) || std::cmp_not_equal(mntRead, size)) {
            std::cerr << "[test] pread failed at offset=" << offset << " size=" << size
                      << " origRead=" << origRead << " mntRead=" << mntRead << "\n";
            close(origFd);
            close(mntFd);
            return false;
        }
        if (std::memcmp(origBuf.data(), mntBuf.data(), size) != 0) {
            std::cerr << "[test] mismatch at offset=" << offset << " size=" << size << "\n";
            close(origFd);
            close(mntFd);
            return false;
        }
    }

    close(origFd);
    close(mntFd);
    return true;
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

    std::this_thread::sleep_for(10ms);

    // Client thread: connects to server and mounts FUSE at MountPoint
    std::jthread clientThread([&testResult, &stopSource](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", ClientPort));
        const ConnectionAddr serverAddr("127.0.0.1", ServerPort);

        const IConnection::Ptr dstConnection = dst.Connect(serverAddr);

        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);
        IPacket::List sendPackets = UDPQueue::CreateBuffers(200000);
        dstConnection->AddFreeRecvPackets(std::move(recvPackets));
        dstConnection->AddFreeSendPackets(std::move(sendPackets));

        FileTree fileTree("/tmp/rfs_client_tree", CacheDir);
        // filesystem must outlive scheduler (scheduler's workers call fuse_reply_*).
        auto filesystem = std::make_unique<RemoteFileSystem>(MountPoint);
        auto scheduler = std::make_unique<TaskScheduler>(*dstConnection, fileTree);
        scheduler->Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));

        RemoteFileSystem::scheduler = scheduler.get();
        filesystem->Start(stop);

        std::jthread readThread([&testResult, &stopSource](std::stop_token /*stop*/) {
            const std::string mntFile = std::string(MountPoint) + "/" + TestFileName;

            // First: direct read comparison
            if (!CompareFiles(TestFilePath, mntFile, TestFileSize)) {
                std::cerr << "[test] first comparison failed\n";
                testResult = false;
                stopSource.request_stop();
                Unmount();
                return;
            }

            // Second: copy via FUSE and verify
            if (!CopyFileViaMount(mntFile, CopyFilePath, "copy_file")) {
                testResult = false;
                stopSource.request_stop();
                Unmount();
                return;
            }
            if (!CompareFiles(TestFilePath, CopyFilePath, TestFileSize)) {
                std::cerr << "[test] copy comparison failed\n";
                testResult = false;
                Unmount();
                return;
            }

            // Third: copy again to verify re-reading works
            if (!CopyFileViaMount(mntFile, Copy2FilePath, "copy2_file")) {
                testResult = false;
                stopSource.request_stop();
                Unmount();
                return;
            }
            const bool result = CompareFiles(TestFilePath, Copy2FilePath, TestFileSize);
            testResult = result;
            stopSource.request_stop();
            Unmount();
        });

        readThread.join();

        scheduler->Wait(stop);
        scheduler.reset();
        filesystem.reset();
        // fileTree, dstConnection, dst (FastTransportContext) destructors follow at end of scope
    });

    // Wait for readThread (inside clientThread) to signal done (max 300s)
    auto deadline = std::chrono::steady_clock::now() + 300s;
    while (!stopSource.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }

    if (!stopSource.stop_requested()) {
        stopSource.request_stop();
        Unmount();
    }

    serverThread.request_stop();
    clientThread.request_stop();
    clientThread.join();
    serverThread.join();

    EXPECT_TRUE(testResult) << "File read via FUSE did not match original ("
                            << TestFileSize << " bytes expected)";
}

TEST(RemoteFileSystemTest, ReadFileRandomAccess) // NOLINT(readability-function-cognitive-complexity)
{
    std::filesystem::create_directories(MountPoint2);
    std::filesystem::remove_all(CacheDir2);
    std::filesystem::create_directories(CacheDir2);
    Unmount(); // clean up MountPoint if left over
    // Ensure test file exists (may have been created by prior test)
    if (!std::filesystem::exists(TestFilePath)) {
        CreateTestFile();
    }

    std::atomic<bool> testResult { false };
    std::stop_source stopSource;

    std::jthread serverThread([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", ServerPort2));
        const IConnection::Ptr srcConnection = src.Accept(stop);
        if (srcConnection == nullptr) {
            return;
        }
        srcConnection->AddFreeRecvPackets(UDPQueue::CreateBuffers(260000));
        srcConnection->AddFreeSendPackets(UDPQueue::CreateBuffers(200000));
        FileTree fileTree("/tmp", CacheDir2);
        TaskScheduler scheduler(*srcConnection, fileTree);
        scheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));
        scheduler.Wait(stop);
    });

    std::this_thread::sleep_for(500ms);

    std::jthread clientThread([&testResult, &stopSource](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", ClientPort2));
        const IConnection::Ptr dstConnection = dst.Connect(ConnectionAddr("127.0.0.1", ServerPort2));
        dstConnection->AddFreeRecvPackets(UDPQueue::CreateBuffers(260000));
        dstConnection->AddFreeSendPackets(UDPQueue::CreateBuffers(200000));

        FileTree fileTree("/tmp/rfs_client_tree2", CacheDir2);
        auto filesystem = std::make_unique<RemoteFileSystem>(MountPoint2);
        auto scheduler = std::make_unique<TaskScheduler>(*dstConnection, fileTree);
        scheduler->Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));
        RemoteFileSystem::scheduler = scheduler.get();
        filesystem->Start(stop);

        std::jthread readThread([&testResult, &stopSource](std::stop_token /*stop*/) {
            const std::string mntFile = std::string(MountPoint2) + "/" + TestFileName;
            const bool result = RandomReadComparison(TestFilePath, mntFile, TestFileSize);
            if (!result) {
                std::cerr << "[test] random read comparison failed\n";
            }
            testResult = result;
            stopSource.request_stop();
            // Unmount MountPoint2
            const pid_t pid = fork();
            if (pid == 0) {
                std::string prog = "fusermount3";
                std::string arg1 = "-u";
                std::string arg2 = MountPoint2;
                std::array<char*, 4> args = { prog.data(), arg1.data(), arg2.data(), nullptr };
                execvp(args[0], args.data());
                _exit(1);
            }
            if (pid > 0) {
                waitpid(pid, nullptr, 0);
            }
        });

        readThread.join();
        scheduler->Wait(stop);
        scheduler.reset();
        filesystem.reset();
    });

    auto deadline = std::chrono::steady_clock::now() + 300s;
    while (!stopSource.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }

    if (!stopSource.stop_requested()) {
        stopSource.request_stop();
    }

    serverThread.request_stop();
    clientThread.request_stop();
    clientThread.join();
    serverThread.join();

    EXPECT_TRUE(testResult) << "Random read via FUSE did not match original";
}

TEST(RemoteFileSystemTest, ParallelListAndReadFiles) // NOLINT(readability-function-cognitive-complexity)
{
    std::filesystem::create_directories(MountPoint3);
    std::filesystem::remove_all(CacheDir3);
    std::filesystem::create_directories(CacheDir3);
    Unmount3();
    CreateTestTree();

    std::atomic<int> failures { 0 };
    std::stop_source stopSource;

    // Server: serves files from TestTreeRoot
    std::jthread serverThread([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", ServerPort3));
        const IConnection::Ptr srcConnection = src.Accept(stop);
        if (srcConnection == nullptr) {
            return;
        }
        srcConnection->AddFreeRecvPackets(UDPQueue::CreateBuffers(260000));
        srcConnection->AddFreeSendPackets(UDPQueue::CreateBuffers(200000));
        FileTree fileTree(TestTreeRoot, CacheDir3);
        TaskScheduler scheduler(*srcConnection, fileTree);
        scheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));
        scheduler.Wait(stop);
    });

    std::this_thread::sleep_for(10ms);

    // Client: mounts FUSE, then spawns parallel threads
    std::jthread clientThread([&failures, &stopSource](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", ClientPort3));
        const IConnection::Ptr dstConnection = dst.Connect(ConnectionAddr("127.0.0.1", ServerPort3));
        dstConnection->AddFreeRecvPackets(UDPQueue::CreateBuffers(260000));
        dstConnection->AddFreeSendPackets(UDPQueue::CreateBuffers(200000));

        FileTree fileTree("/tmp/rfs_client_tree3", CacheDir3);
        auto filesystem = std::make_unique<RemoteFileSystem>(MountPoint3);
        auto scheduler = std::make_unique<TaskScheduler>(*dstConnection, fileTree);
        scheduler->Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));
        RemoteFileSystem::scheduler = scheduler.get();
        filesystem->Start(stop);

        const std::string mntRoot = MountPoint3;

        // Thread 1: list directories recursively until stopped
        std::jthread listThread([&failures, &mntRoot](std::stop_token stop) {
            auto listDir = [&failures](const std::string& path, auto& self) -> void {
                DIR* dir = opendir(path.c_str());
                if (dir == nullptr) {
                    std::cerr << "[test] opendir failed: " << path << "\n";
                    ++failures;
                    return;
                }
                while (struct dirent* entry = readdir(dir)) { // NOLINT(concurrency-mt-unsafe)
                    const std::string name = entry->d_name;
                    if (name == "." || name == "..") {
                        continue;
                    }
                    const std::string fullPath = path + "/" + name;
                    std::cerr << "[test] listed: " << fullPath << "\n";
                    if (entry->d_type == DT_DIR) {
                        self(fullPath, self);
                    }
                }
                closedir(dir);
            };
            int iteration = 0;
            while (!stop.stop_requested()) {
                listDir(mntRoot, listDir);
                ++iteration;
            }
            std::cerr << "[test] listThread did " << iteration << " iterations\n";
        });

        // Thread 2: read file_a.bin (300 MB) until stopped
        std::jthread readAThread([&failures, &mntRoot](std::stop_token stop) {
            const std::string origPath = std::string(TestTreeRoot) + "/subdir_a/file_a.bin";
            const std::string mntPath = mntRoot + "/subdir_a/file_a.bin";
            int iteration = 0;
            while (!stop.stop_requested()) {
                if (!CompareFiles(origPath, mntPath, FileASize)) {
                    std::cerr << "[test] file_a.bin comparison failed\n";
                    ++failures;
                    return;
                }

                LOGGER() << "[test] readAThread iteration " << iteration << " succeeded\n";
                ++iteration;
            }
            std::cerr << "[test] readAThread did " << iteration << " iterations\n";
        });

        // Thread 3: read file_b.bin (200 MB) until stopped
        std::jthread readBThread([&failures, &mntRoot](std::stop_token stop) {
            const std::string origPath = std::string(TestTreeRoot) + "/subdir_b/file_b.bin";
            const std::string mntPath = mntRoot + "/subdir_b/file_b.bin";
            int iteration = 0;
            while (!stop.stop_requested()) {
                if (!CompareFiles(origPath, mntPath, FileBSize)) {
                    std::cerr << "[test] file_b.bin comparison failed\n";
                    ++failures;
                    return;
                }
                LOGGER() << "[test] readBThread iteration " << iteration << " succeeded\n";
                ++iteration;
            }
            std::cerr << "[test] readBThread did " << iteration << " iterations\n";
        });

        // Thread 4: read file_root.bin (100 MB) until stopped
        std::jthread readRootThread([&failures, &mntRoot](std::stop_token stop) {
            const std::string origPath = std::string(TestTreeRoot) + "/file_root.bin";
            const std::string mntPath = mntRoot + "/file_root.bin";
            int iteration = 0;
            while (!stop.stop_requested()) {
                if (!CompareFiles(origPath, mntPath, FileRootSize)) {
                    std::cerr << "[test] file_root.bin comparison failed\n";
                    ++failures;
                    return;
                }
                LOGGER() << "[test] readRootThread iteration " << iteration << " succeeded\n";
                ++iteration;
            }
            std::cerr << "[test] readRootThread did " << iteration << " iterations\n";
        });

        std::this_thread::sleep_for(5s);
        listThread.request_stop();
        readAThread.request_stop();
        readBThread.request_stop();
        readRootThread.request_stop();

        LOGGER() << "[test] requested stop for all threads, waiting for them to finish...\n";
        listThread.join();
        LOGGER() << "[test] listThread stopped\n";
        readAThread.join();
        LOGGER() << "[test] readAThread stopped\n";
        readBThread.join();
        LOGGER() << "[test] readBThread stopped\n";
        readRootThread.join();
        LOGGER() << "[test] all threads stopped\n";


        std::this_thread::sleep_for(1s);
        stopSource.request_stop();
        Unmount3();

        scheduler->Wait(stop);
        scheduler.reset();
        filesystem.reset();
    });

    auto deadline = std::chrono::steady_clock::now() + 300s;
    while (!stopSource.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }

    if (!stopSource.stop_requested()) {
        stopSource.request_stop();
        Unmount3();
    }

    serverThread.request_stop();
    clientThread.request_stop();
    clientThread.join();
    serverThread.join();

    EXPECT_EQ(failures.load(), 0) << "Parallel list/read test had failures";
}

#endif // __linux__
