#include "FuseRequestTracker.hpp"
#include <gtest/gtest.h>

#ifdef __linux__

#include <atomic>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <stop_token>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#include "jprocess.hpp"

#include "ConnectionAddr.hpp"
#include "FastTransportProtocol.hpp"
#include "FileSystem/RemoteFileSystem.hpp"
#include "IPacket.hpp"
#include "InotifyWatcherJob.hpp"
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

// Directory structure for multi-process test:
// /tmp/rfs_test_tree/
//   ├── subdir_a/
//   │   └── file_a.bin   (600 MB)
//   ├── subdir_b/
//   │   └── file_b.bin   (300 MB)
//   └── file_root.bin    (300 MB)
constexpr const char* TestTreeRoot = "/tmp/rfs_test_tree";
constexpr size_t FileASize = 600ULL * 1024 * 1024;
constexpr size_t FileBSize = 300ULL * 1024 * 1024;
constexpr size_t FileRootSize = 300ULL * 1024 * 1024;

constexpr uint16_t ServerPort4 = 31700;
constexpr uint16_t ClientPort4 = 31800;
constexpr const char* MountPoint4 = "/tmp/mnt_notify_create";
constexpr const char* CacheDir4 = "/tmp/notify_create_cache";
constexpr const char* ServerRoot4 = "/tmp/notify_create_server";

constexpr uint16_t ServerPort5 = 31900;
constexpr uint16_t ClientPort5 = 32000;
constexpr const char* MountPoint5 = "/tmp/mnt_notify_delete";
constexpr const char* CacheDir5 = "/tmp/notify_delete_cache";
constexpr const char* ServerRoot5 = "/tmp/notify_delete_server";

constexpr uint16_t ServerPort6 = 32100;
constexpr uint16_t ClientPort6 = 32200;
constexpr const char* MountPoint6 = "/tmp/mnt_notify_modify";
constexpr const char* CacheDir6 = "/tmp/notify_modify_cache";
constexpr const char* ServerRoot6 = "/tmp/notify_modify_server";

// Block size must match Leaf::BlockSize (1000 * 1300)
constexpr size_t FuseBlockSize = 1000UL * 1300UL;

// ── file creation ─────────────────────────────────────────────────────────────

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

// ── mount / directory helpers ─────────────────────────────────────────────────

// Runs fusermount3 -u in a child process so unmounting works from a thread/process
// that did not mount the filesystem.
void RunFusermount(const char* mountPoint, const char* flag)
{
    const pid_t pid = fork();
    if (pid == 0) {
        std::string prog = "fusermount3";
        std::string arg1 = flag;
        std::string arg2 = mountPoint;
        std::array<char*, 4> args = { prog.data(), arg1.data(), arg2.data(), nullptr };
        execvp(args[0], args.data());
        _exit(1);
    }
    if (pid > 0) {
        waitpid(pid, nullptr, 0);
    }
}

void UnmountAt(const char* mountPoint)
{
    RunFusermount(mountPoint, "-u");
}

void LazyUnmountAt(const char* mountPoint)
{
    RunFusermount(mountPoint, "-uz");
}

void PrepareTestDirectories(const char* mountPoint, const char* cacheDir)
{
    UnmountAt(mountPoint); // clean up any leftover mount from a previous run
    std::filesystem::create_directories(mountPoint);
    std::filesystem::remove_all(cacheDir);
    std::filesystem::create_directories(cacheDir);
}

// ── file comparison ───────────────────────────────────────────────────────────

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

bool CompareFiles(const std::string& path1, const std::string& path2, size_t totalSize, jprocess::stop_token stop)
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
    while (totalRead < totalSize && !stop.stop_requested()) {
        const size_t remaining = totalSize - totalRead;
        const auto toRead = static_cast<std::streamsize>(std::min(ChunkSize, remaining));
        if (!file1.read(buf1.data(), toRead) || file1.gcount() != toRead || stop.stop_requested()) {
            return false;
        }
        if (!file2.read(buf2.data(), toRead) || file2.gcount() != toRead || stop.stop_requested()) {
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

// ── server helper ─────────────────────────────────────────────────────────────

void RunServer(std::stop_token stop, uint16_t port, const char* root, const char* cacheDir)
{
    FastTransportContext src(ConnectionAddr("127.0.0.1", port));
    const IConnection::Ptr srcConnection = src.Accept(stop);
    if (srcConnection == nullptr) {
        return;
    }
    srcConnection->AddFreeRecvPackets(UDPQueue::CreateBuffers(260000));
    srcConnection->AddFreeSendPackets(UDPQueue::CreateBuffers(200000));
    FileTree fileTree(root, cacheDir);
    TaskScheduler scheduler(*srcConnection, fileTree);
    scheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));
    scheduler.Wait(stop);
}

void RunServerWithInotify(std::stop_token stop, uint16_t port, const char* root, const char* cacheDir)
{
    FastTransportContext src(ConnectionAddr("127.0.0.1", port));
    const IConnection::Ptr srcConnection = src.Accept(stop);
    if (srcConnection == nullptr) {
        return;
    }
    srcConnection->AddFreeRecvPackets(UDPQueue::CreateBuffers(260000));
    srcConnection->AddFreeSendPackets(UDPQueue::CreateBuffers(200000));
    FileTree fileTree(root, cacheDir);
    TaskScheduler scheduler(*srcConnection, fileTree);
    scheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));
    scheduler.Schedule(std::make_unique<FastTransport::TaskQueue::InotifyWatcherJob>(root, fileTree, scheduler));
    scheduler.Wait(stop);
}

size_t CountDirEntries(const std::string& path)
{
    size_t count = 0;
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        return 0;
    }
    while (struct dirent* entry = readdir(dir)) { // NOLINT(concurrency-mt-unsafe)
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
        const std::string_view name(entry->d_name);
        if (name != "." && name != "..") {
            ++count;
        }
    }
    closedir(dir);
    return count;
}

// ── directory listing helper ──────────────────────────────────────────────────

bool ListDirectoriesIterative(const std::string& root)
{
    std::vector<std::string> stack = { root };
    while (!stack.empty()) {
        const std::string path = std::move(stack.back());
        stack.pop_back();

        DIR* dir = opendir(path.c_str());
        if (dir == nullptr) {
            std::cerr << "[test] opendir failed: " << path << "\n";
            return false;
        }
        while (struct dirent* entry = readdir(dir)) { // NOLINT(concurrency-mt-unsafe)
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            const std::string_view name(entry->d_name);
            if (name == "." || name == "..") {
                continue;
            }
            std::string fullPath = path;
            fullPath += '/';
            fullPath += name;
            std::cerr << "[test] listed: " << fullPath << "\n";
            if (entry->d_type == DT_DIR) {
                stack.push_back(std::move(fullPath));
            }
        }
        closedir(dir);
    }
    return true;
}

// Loop helpers: intended to run inside a jprocess child.

bool ListDirectoriesLoop(const std::string& root, jprocess::stop_token stop)
{
    constexpr int NumIterations = 10;
    for (int iteration = 0; iteration < NumIterations; ++iteration) {
        if (stop.stop_requested()) {
            std::cerr << "[test] listProcess stopped at iteration " << iteration << "\n";
            return false;
        }
        if (!ListDirectoriesIterative(root)) {
            std::cerr << "[test] listProcess failed at iteration " << iteration << "\n";
            return false;
        }
    }
    std::cerr << "[test] listProcess completed " << NumIterations << " iterations\n";
    return true;
}

bool ReadFileComparisonLoop(const std::string& orig, const std::string& mnt,
    size_t size, const char* name, jprocess::stop_token stop)
{
    constexpr int NumIterations = 5;
    for (int iteration = 0; iteration < NumIterations; ++iteration) {
        if (stop.stop_requested()) {
            std::cerr << "[test] " << name << " stopped at iteration " << iteration << "\n";
            return false;
        }
        if (!CompareFiles(orig, mnt, size, stop)) {
            std::cerr << "[test] " << name << " comparison failed at iteration " << iteration << "\n";
            return false;
        }
    }
    std::cerr << "[test] " << name << " completed " << NumIterations << " iterations\n";
    return true;
}

} // namespace

TEST(RemoteFileSystemTest, ReadFileOverNetwork) // NOLINT(readability-function-cognitive-complexity)
{
    PrepareTestDirectories(MountPoint, CacheDir);
    CreateTestFile();

    std::atomic<bool> testResult { false };
    std::stop_source stopSource;

    std::jthread serverThread([](std::stop_token stop) {
        RunServer(stop, ServerPort, "/tmp", CacheDir);
    });

    std::this_thread::sleep_for(10ms);

    std::jthread clientThread([&testResult, &stopSource](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", ClientPort));
        const IConnection::Ptr dstConnection = dst.Connect(ConnectionAddr("127.0.0.1", ServerPort));
        dstConnection->AddFreeRecvPackets(UDPQueue::CreateBuffers(260000));
        dstConnection->AddFreeSendPackets(UDPQueue::CreateBuffers(200000));

        FileTree fileTree("/tmp/rfs_client_tree", CacheDir);
        // filesystem must outlive scheduler (scheduler's workers call fuse_reply_*).
        auto filesystem = std::make_unique<RemoteFileSystem>(MountPoint);
        auto scheduler = std::make_unique<TaskScheduler>(*dstConnection, fileTree);
        scheduler->Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));
        RemoteFileSystem::scheduler = scheduler.get();
        filesystem->Start(stop);

        const std::string mntFile = std::string(MountPoint) + "/" + TestFileName;
        jprocess proc([&mntFile](jprocess::stop_token /*stop*/) -> bool {
            if (!CompareFiles(TestFilePath, mntFile, TestFileSize)) {
                std::cerr << "[test] first comparison failed\n";
                return false;
            }
            if (!CopyFileViaMount(mntFile, CopyFilePath, "copy_file")) {
                return false;
            }
            if (!CompareFiles(TestFilePath, CopyFilePath, TestFileSize)) {
                std::cerr << "[test] copy comparison failed\n";
                return false;
            }
            if (!CopyFileViaMount(mntFile, Copy2FilePath, "copy2_file")) {
                return false;
            }
            return CompareFiles(TestFilePath, Copy2FilePath, TestFileSize);
        });
        testResult = proc.join();

        stopSource.request_stop();
        UnmountAt(MountPoint);

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
        UnmountAt(MountPoint);
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
    PrepareTestDirectories(MountPoint2, CacheDir2);
    if (!std::filesystem::exists(TestFilePath)) {
        CreateTestFile();
    }

    std::atomic<bool> testResult { false };
    std::stop_source stopSource;

    std::jthread serverThread([](std::stop_token stop) {
        RunServer(stop, ServerPort2, "/tmp", CacheDir2);
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

        const std::string mntFile = std::string(MountPoint2) + "/" + TestFileName;
        jprocess proc([&mntFile](jprocess::stop_token /*stop*/) -> bool {
            const bool result = RandomReadComparison(TestFilePath, mntFile, TestFileSize);
            if (!result) {
                std::cerr << "[test] random read comparison failed\n";
            }
            return result;
        });
        testResult = proc.join();

        stopSource.request_stop();
        UnmountAt(MountPoint2);

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
    PrepareTestDirectories(MountPoint3, CacheDir3);
    CreateTestTree();

    std::atomic<bool> testResult { false };
    std::stop_source stopSource;

    std::jthread serverThread([](std::stop_token stop) {
        RunServer(stop, ServerPort3, TestTreeRoot, CacheDir3);
    });

    std::this_thread::sleep_for(10ms);

    std::jthread clientThread([&testResult, &stopSource](std::stop_token stop) {
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
        const std::string origA = std::string(TestTreeRoot) + "/subdir_a/file_a.bin";
        const std::string mntA = mntRoot + "/subdir_a/file_a.bin";
        const std::string origB = std::string(TestTreeRoot) + "/subdir_b/file_b.bin";
        const std::string mntB = mntRoot + "/subdir_b/file_b.bin";
        const std::string origRootFile = std::string(TestTreeRoot) + "/file_root.bin";
        const std::string mntRootFile = mntRoot + "/file_root.bin";

        // Each operation runs in its own child process so all proceed in parallel.
        jprocess procList([&mntRoot](jprocess::stop_token stop) -> bool {
            return ListDirectoriesLoop(mntRoot, stop);
        });
        jprocess procFileA([&origA, &mntA](jprocess::stop_token stop) -> bool {
            return ReadFileComparisonLoop(origA, mntA, FileASize, "file_a.bin", stop);
        });
        jprocess procFileB([&origB, &mntB](jprocess::stop_token stop) -> bool {
            return ReadFileComparisonLoop(origB, mntB, FileBSize, "file_b.bin", stop);
        });
        jprocess procRoot([&origRootFile, &mntRootFile](jprocess::stop_token stop) -> bool {
            return ReadFileComparisonLoop(origRootFile, mntRootFile, FileRootSize, "file_root.bin", stop);
        });

        // All processes run a fixed number of iterations and exit on their own.
        // Wait for all four to complete (with a safety timeout).
        static constexpr auto TestTimeout = std::chrono::seconds(120);
        std::optional<bool> listResult;
        std::optional<bool> fileAResult;
        std::optional<bool> fileBResult;
        std::optional<bool> fileRootResult;
        const auto testDeadline = std::chrono::steady_clock::now() + TestTimeout;
        while (std::chrono::steady_clock::now() < testDeadline) {
            if (!listResult) {
                listResult = procList.tryJoin();
            }
            if (!fileAResult) {
                fileAResult = procFileA.tryJoin();
            }
            if (!fileBResult) {
                fileBResult = procFileB.tryJoin();
            }
            if (!fileRootResult) {
                fileRootResult = procRoot.tryJoin();
            }
            if (listResult && fileAResult && fileBResult && fileRootResult) {
                break;
            }
            std::this_thread::sleep_for(50ms);
        }

        const bool anyStuck = !listResult || !fileAResult || !fileBResult || !fileRootResult;
        if (anyStuck) {
            // A process is stuck in a FUSE syscall (e.g. read waiting for a block that
            // never arrives). The stop flag cannot unblock a process waiting inside the
            // kernel. Close the FUSE session fd to send ENODEV to all blocked syscalls.
            //
            // Order matters: stop FUSE callbacks first so they don't race with scheduler
            // teardown, then drain the scheduler, cancel pending requests, destroy the fd.
            std::cerr << "[shutdown] stuck:"
                      << (!listResult ? " procList" : "")
                      << (!fileAResult ? " procFileA" : "")
                      << (!fileBResult ? " procFileB" : "")
                      << (!fileRootResult ? " procRoot" : "")
                      << " — forcing shutdown\n";
            // Signal stuck processes to exit via stop token before we tear down FUSE.
            procList.request_stop();
            procFileA.request_stop();
            procFileB.request_stop();
            procRoot.request_stop();
            filesystem->RequestStopAndWait();
            scheduler.reset();
            fileTree.CancelAllPendingJobs();
            filesystem.reset();
        }

        std::cerr << "[shutdown] joining procList\n";
        const bool listOk = listResult.value_or(procList.join());
        std::cerr << "[shutdown] procList joined: " << listOk << "\n";

        std::cerr << "[shutdown] joining procFileA\n";
        const bool fileAOk = fileAResult.value_or(procFileA.join());
        std::cerr << "[shutdown] procFileA joined: " << fileAOk << "\n";

        std::cerr << "[shutdown] joining procFileB\n";
        const bool fileBOk = fileBResult.value_or(procFileB.join());
        std::cerr << "[shutdown] procFileB joined: " << fileBOk << "\n";

        std::cerr << "[shutdown] joining procRoot\n";
        const bool fileRootOk = fileRootResult.value_or(procRoot.join());
        std::cerr << "[shutdown] procRoot joined: " << fileRootOk << "\n";

        testResult = listOk && fileAOk && fileBOk && fileRootOk;
        std::cerr << "[shutdown] testResult=" << testResult
                  << " (list=" << listOk << " fileA=" << fileAOk
                  << " fileB=" << fileBOk << " fileRoot=" << fileRootOk << ")\n";

        stopSource.request_stop();

        if (!anyStuck) {
            UnmountAt(MountPoint3);
            scheduler->Wait(stop);
            scheduler.reset();
            filesystem.reset();
        }
    });

    auto deadline = std::chrono::steady_clock::now() + 130s;
    while (!stopSource.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }

    if (!stopSource.stop_requested()) {
        // Safety fallback: the client thread is stuck (should not happen normally).
        std::cerr << "[shutdown] deadline exceeded\n";
        stopSource.request_stop();
        LazyUnmountAt(MountPoint3);
        std::cerr << "[shutdown] lazy unmount done\n";
    }

    std::cerr << "[shutdown] requesting clientThread stop\n";
    clientThread.request_stop();
    std::cerr << "[shutdown] joining clientThread\n";
    clientThread.join();
    std::cerr << "[shutdown] clientThread joined\n";

    std::cerr << "[shutdown] requesting serverThread stop\n";
    serverThread.request_stop();
    std::cerr << "[shutdown] joining serverThread\n";
    serverThread.join();
    std::cerr << "[shutdown] serverThread joined\n";

    EXPECT_TRUE(testResult) << "Parallel list/read test had failures";
}

namespace {

// Helper: sets up client (connect, FUSE start, session), runs body(stop), then tears down.
// body should signal stopSource when done.
template <typename Body>
void RunClientWithNotify(std::stop_token stop, uint16_t port, const char* mountPoint, const char* cacheDir,
    const char* serverRoot, std::stop_source& stopSource, std::atomic<bool>& result, Body&& body)
{
    FastTransportContext dst(ConnectionAddr("127.0.0.1", port));
    const IConnection::Ptr dstConnection = dst.Connect(ConnectionAddr("127.0.0.1", static_cast<uint16_t>(port - 100)));
    dstConnection->AddFreeRecvPackets(UDPQueue::CreateBuffers(260000));
    dstConnection->AddFreeSendPackets(UDPQueue::CreateBuffers(200000));

    FileTree fileTree(serverRoot, cacheDir);
    auto filesystem = std::make_unique<RemoteFileSystem>(mountPoint);
    auto scheduler = std::make_unique<TaskScheduler>(*dstConnection, fileTree);
    scheduler->Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));
    RemoteFileSystem::scheduler = scheduler.get();
    filesystem->Init();
    RemoteFileSystem::session = filesystem->GetSession();
    filesystem->Start(stop);

    result = std::forward<Body>(body)(stop, fileTree, mountPoint);

    RemoteFileSystem::session = nullptr;
    stopSource.request_stop();
    UnmountAt(mountPoint);
    scheduler->Wait(stop);
    scheduler.reset();
    filesystem.reset();
}

} // namespace

TEST(RemoteFileSystemTest, NotifyInvalEntryOnFileCreate) // NOLINT(readability-function-cognitive-complexity)
{
    std::filesystem::remove_all(ServerRoot4);
    std::filesystem::create_directories(std::string(ServerRoot4) + "/watched");
    PrepareTestDirectories(MountPoint4, CacheDir4);

    std::atomic<bool> testResult { false };
    std::stop_source stopSource;

    std::jthread serverThread([](std::stop_token stop) {
        RunServerWithInotify(stop, ServerPort4, ServerRoot4, CacheDir4);
    });

    std::this_thread::sleep_for(10ms);

    std::jthread clientThread([&testResult, &stopSource](std::stop_token stop) {
        RunClientWithNotify(stop, ClientPort4, MountPoint4, CacheDir4, ServerRoot4, stopSource, testResult,
            [](std::stop_token /*stop*/, FastTransport::FileSystem::FileTree& /*tree*/, const char* mountPoint) -> bool {
                const std::string watchedMount = std::string(mountPoint) + "/watched";

                // Initial ls: watched/ should be empty (also seeds serverInode for watched/).
                jprocess checkEmpty([&watchedMount](jprocess::stop_token /*s*/) -> bool {
                    return CountDirEntries(watchedMount) == 0;
                });
                if (!checkEmpty.join()) {
                    std::cerr << "[test] initial listing was not empty\n";
                    return false;
                }

                // Create a file on the server inside watched/.
                {
                    std::ofstream(ServerRoot4 + std::string("/watched/new_file.txt")) << "hello";
                }

                // Poll until the client mount reflects the new entry.
                constexpr auto Timeout = std::chrono::seconds(3);
                const auto deadline = std::chrono::steady_clock::now() + Timeout;
                jprocess poll([deadline, &watchedMount](jprocess::stop_token pollStop) -> bool {
                    while (std::chrono::steady_clock::now() < deadline && !pollStop.stop_requested()) {
                        if (CountDirEntries(watchedMount) == 1) {
                            return true;
                        }
                        std::this_thread::sleep_for(50ms);
                    }
                    return false;
                });
                return poll.join();
            });
    });

    auto deadline = std::chrono::steady_clock::now() + 30s;
    while (!stopSource.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }
    if (!stopSource.stop_requested()) {
        stopSource.request_stop();
        LazyUnmountAt(MountPoint4);
    }
    serverThread.request_stop();
    clientThread.request_stop();
    clientThread.join();
    serverThread.join();

    EXPECT_TRUE(testResult) << "File created on server was not visible via FUSE mount within timeout";
}

TEST(RemoteFileSystemTest, NotifyInvalEntryOnFileDelete) // NOLINT(readability-function-cognitive-complexity)
{
    std::filesystem::remove_all(ServerRoot5);
    std::filesystem::create_directories(std::string(ServerRoot5) + "/watched");
    {
        std::ofstream(std::string(ServerRoot5) + "/watched/existing.txt") << "data";
    }
    PrepareTestDirectories(MountPoint5, CacheDir5);

    std::atomic<bool> testResult { false };
    std::stop_source stopSource;

    std::jthread serverThread([](std::stop_token stop) {
        RunServerWithInotify(stop, ServerPort5, ServerRoot5, CacheDir5);
    });

    std::this_thread::sleep_for(10ms);

    std::jthread clientThread([&testResult, &stopSource](std::stop_token stop) {
        RunClientWithNotify(stop, ClientPort5, MountPoint5, CacheDir5, ServerRoot5, stopSource, testResult,
            [](std::stop_token /*stop*/, FastTransport::FileSystem::FileTree& /*tree*/, const char* mountPoint) -> bool {
                const std::string watchedMount = std::string(mountPoint) + "/watched";

                // Initial ls: 1 entry, also seeds serverInode for watched/ and existing.txt.
                jprocess checkOne([&watchedMount](jprocess::stop_token /*s*/) -> bool {
                    return CountDirEntries(watchedMount) == 1;
                });
                if (!checkOne.join()) {
                    std::cerr << "[test] initial listing did not have 1 entry\n";
                    return false;
                }

                // Delete the file on the server.
                std::filesystem::remove(std::string(ServerRoot5) + "/watched/existing.txt");

                // Poll until the client mount reflects the deletion.
                constexpr auto Timeout = std::chrono::seconds(3);
                const auto deadline = std::chrono::steady_clock::now() + Timeout;
                jprocess poll([deadline, &watchedMount](jprocess::stop_token pollStop) -> bool {
                    while (std::chrono::steady_clock::now() < deadline && !pollStop.stop_requested()) {
                        if (CountDirEntries(watchedMount) == 0) {
                            return true;
                        }
                        std::this_thread::sleep_for(50ms);
                    }
                    return false;
                });
                return poll.join();
            });
    });

    auto deadline = std::chrono::steady_clock::now() + 30s;
    while (!stopSource.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }
    if (!stopSource.stop_requested()) {
        stopSource.request_stop();
        LazyUnmountAt(MountPoint5);
    }
    serverThread.request_stop();
    clientThread.request_stop();
    clientThread.join();
    serverThread.join();

    EXPECT_TRUE(testResult) << "File deleted on server was not removed from FUSE mount within timeout";
}

TEST(RemoteFileSystemTest, NotifyInvalInodeOnFileModify) // NOLINT(readability-function-cognitive-complexity)
{
    constexpr size_t FileSize = 4096;
    std::filesystem::remove_all(ServerRoot6);
    std::filesystem::create_directories(std::string(ServerRoot6) + "/watched");
    {
        std::ofstream file(std::string(ServerRoot6) + "/watched/data.bin", std::ios::binary);
        const std::vector<char> zeros(FileSize, '\0');
        file.write(zeros.data(), static_cast<std::streamsize>(FileSize));
    }
    PrepareTestDirectories(MountPoint6, CacheDir6);

    std::atomic<bool> testResult { false };
    std::stop_source stopSource;

    std::jthread serverThread([](std::stop_token stop) {
        RunServerWithInotify(stop, ServerPort6, ServerRoot6, CacheDir6);
    });

    std::this_thread::sleep_for(10ms);

    std::jthread clientThread([&testResult, &stopSource](std::stop_token stop) {
        RunClientWithNotify(stop, ClientPort6, MountPoint6, CacheDir6, ServerRoot6, stopSource, testResult,
            [](std::stop_token /*stop*/, FastTransport::FileSystem::FileTree& /*tree*/, const char* mountPoint) -> bool {
                const std::string dataFile = std::string(mountPoint) + "/watched/data.bin";
                constexpr size_t FileSize = 4096;

                // Initial read: seeds serverInodes for watched/ and data.bin, verifies zeros.
                jprocess checkZeros([&dataFile](jprocess::stop_token /*s*/) -> bool {
                    std::ifstream file(dataFile, std::ios::binary);
                    if (!file) {
                        return false;
                    }
                    std::vector<char> buf(FileSize);
                    if (!file.read(buf.data(), static_cast<std::streamsize>(FileSize))) {
                        return false;
                    }
                    return buf[0] == '\0' && buf[FileSize - 1] == '\0';
                });
                if (!checkZeros.join()) {
                    std::cerr << "[test] initial content was not zeros\n";
                    return false;
                }

                // Overwrite the file with ones on the server.
                {
                    std::ofstream file(std::string(ServerRoot6) + "/watched/data.bin", std::ios::binary);
                    const std::vector<char> ones(FileSize, '\x01');
                    file.write(ones.data(), static_cast<std::streamsize>(FileSize));
                }

                // Poll until the client reads the new content.
                constexpr auto Timeout = std::chrono::seconds(3);
                const auto deadline = std::chrono::steady_clock::now() + Timeout;
                jprocess poll([deadline, &dataFile](jprocess::stop_token pollStop) -> bool {
                    while (std::chrono::steady_clock::now() < deadline && !pollStop.stop_requested()) {
                        std::ifstream file(dataFile, std::ios::binary);
                        if (file) {
                            std::vector<char> buf(FileSize);
                            if (file.read(buf.data(), static_cast<std::streamsize>(FileSize)) && buf[0] == '\x01') {
                                return true;
                            }
                        }
                        std::this_thread::sleep_for(100ms);
                    }
                    return false;
                });
                return poll.join();
            });
    });

    auto deadline = std::chrono::steady_clock::now() + 30s;
    while (!stopSource.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }
    if (!stopSource.stop_requested()) {
        stopSource.request_stop();
        LazyUnmountAt(MountPoint6);
    }
    serverThread.request_stop();
    clientThread.request_stop();
    clientThread.join();
    serverThread.join();

    EXPECT_TRUE(testResult) << "Modified file content was not visible via FUSE mount within timeout";
}

// Functional repro for the kernel-pinned-Leaf lifetime race: real server +
// client + FUSE mount. The server churns files in a watched directory;
// InotifyWatcherJob propagates each delete via NotifyInvalEntry →
// Leaf::RemoveChild on the client. In parallel, a reader process opens/stats
// entries on the mount, which drives fuse_lookup → Leaf::AddRef.
//
// Originally this exercised a UAF where RemoveChild dropped a Leaf still
// referenced by the kernel; the current ownership model (shared_ptr children
// map + _selfPin captured by the first AddRef) keeps the Leaf alive across
// the gap. The test stays in the suite as a regression guard — TSan must see
// no race on Leaf state across the lookup/RemoveChild interleaving, and ASan
// must report no leaks (the dtor's DropPinsRecursively breaks any leftover
// self-reference cycles).
namespace {

constexpr uint16_t ServerPort7 = 32300;
constexpr uint16_t ClientPort7 = 32400;
constexpr const char* MountPoint7 = "/tmp/mnt_leaf_lifetime";
constexpr const char* CacheDir7 = "/tmp/leaf_lifetime_cache";
constexpr const char* ServerRoot7 = "/tmp/leaf_lifetime_server";

constexpr int LeafLifetimeFilesPerRound = 8;
constexpr int LeafLifetimeRounds = 6;
constexpr int LeafLifetimeReaderIterations = 80;

bool LeafLifetimeReaderLoop(const std::string& watchedMount)
{
    for (int it = 0; it < LeafLifetimeReaderIterations; ++it) {
        DIR* dir = opendir(watchedMount.c_str());
        if (dir == nullptr) {
            continue;
        }
        while (struct dirent* dent = readdir(dir)) { // NOLINT(concurrency-mt-unsafe)
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
            const std::string_view name(dent->d_name);
            if (name == "." || name == "..") {
                continue;
            }
            struct stat statBuf {};
            const std::string fullPath = watchedMount + "/" + std::string(name);
            stat(fullPath.c_str(), &statBuf);
        }
        closedir(dir);
        std::this_thread::sleep_for(5ms);
    }
    return true;
}

bool LeafLifetimeMutatorLoop(const std::string& watchedServer)
{
    for (int round = 0; round < LeafLifetimeRounds; ++round) {
        for (int i = 0; i < LeafLifetimeFilesPerRound; ++i) {
            std::ofstream(watchedServer + "/f" + std::to_string(i) + ".txt") << "x";
        }
        std::this_thread::sleep_for(20ms);
        for (int i = 0; i < LeafLifetimeFilesPerRound; ++i) {
            std::filesystem::remove(watchedServer + "/f" + std::to_string(i) + ".txt");
        }
        std::this_thread::sleep_for(20ms);
    }
    return true;
}

bool LeafLifetimeRaceBody(const char* mountPoint)
{
    const std::string watchedMount = std::string(mountPoint) + "/watched";
    const std::string watchedServer = std::string(ServerRoot7) + "/watched";

    // Warmup: ls watched/ so the server records its serverInode (otherwise
    // NotifyInvalEntry from inotify cannot resolve to a client Leaf and
    // RemoveChild is never called → no race window opens).
    jprocess warmup([&watchedMount](jprocess::stop_token /*s*/) -> bool {
        return CountDirEntries(watchedMount) >= 1;
    });
    if (!warmup.join()) {
        std::cerr << "[test] warmup ls failed\n";
        return false;
    }

    // Reader: bounded ls+stat — drives fuse_lookup → AddRef.
    jprocess reader([&watchedMount](jprocess::stop_token /*s*/) -> bool {
        return LeafLifetimeReaderLoop(watchedMount);
    });

    // Mutator: bounded server-side churn — fires inotify → NotifyInvalEntry →
    // Leaf::RemoveChild on the client _mainQueue.
    jprocess mutator([&watchedServer](jprocess::stop_token /*s*/) -> bool {
        return LeafLifetimeMutatorLoop(watchedServer);
    });

    const bool mutatorOk = mutator.join();
    const bool readerOk = reader.join();
    return mutatorOk && readerOk;
}

} // namespace

TEST(RemoteFileSystemTest, LeafLifetimeRaceUnderChurn)
{
    // Bounded reader + bounded mutator (each in its own jprocess) running in
    // parallel: they self-terminate on iteration count, so by the time `body`
    // returns no FUSE syscall is in flight on the mount and the standard
    // RunClientWithNotify teardown (UnmountAt + scheduler.Wait + reset) drains
    // cleanly — same shape as NotifyInvalEntryOnFileDelete and
    // ParallelListAndReadFiles which already work in this codebase.
    std::filesystem::remove_all(ServerRoot7);
    std::filesystem::create_directories(std::string(ServerRoot7) + "/watched");
    {
        std::ofstream(std::string(ServerRoot7) + "/watched/anchor.txt") << "x";
    }
    PrepareTestDirectories(MountPoint7, CacheDir7);

    std::atomic<bool> testResult { false };
    std::stop_source stopSource;

    std::jthread serverThread([](std::stop_token stop) {
        RunServerWithInotify(stop, ServerPort7, ServerRoot7, CacheDir7);
    });

    std::this_thread::sleep_for(50ms);

    std::jthread clientThread([&testResult, &stopSource](std::stop_token stop) {
        RunClientWithNotify(stop, ClientPort7, MountPoint7, CacheDir7, ServerRoot7, stopSource, testResult,
            [](std::stop_token /*stop*/, FastTransport::FileSystem::FileTree& /*tree*/, const char* mountPoint) -> bool {
                return LeafLifetimeRaceBody(mountPoint);
            });
    });

    const auto deadline = std::chrono::steady_clock::now() + 60s;
    while (!stopSource.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }
    if (!stopSource.stop_requested()) {
        std::cerr << "[shutdown] leaf-lifetime deadline exceeded\n";
        stopSource.request_stop();
        LazyUnmountAt(MountPoint7);
    }
    serverThread.request_stop();
    clientThread.request_stop();
    clientThread.join();
    serverThread.join();

    EXPECT_TRUE(testResult) << "LeafLifetime race churn test did not complete cleanly";
}

#endif // __linux__
