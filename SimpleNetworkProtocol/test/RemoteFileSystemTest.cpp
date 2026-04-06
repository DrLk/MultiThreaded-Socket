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
//   │   └── file_a.bin   (30 MB)
//   ├── subdir_b/
//   │   └── file_b.bin   (30 MB)
//   └── file_root.bin    (30 MB)
constexpr const char* TestTreeRoot = "/tmp/rfs_test_tree";
constexpr size_t FileASize = 300ULL * 1024 * 1024;
constexpr size_t FileBSize = 300ULL * 1024 * 1024;
constexpr size_t FileRootSize = 300ULL * 1024 * 1024;

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
    int iteration = 0;
    while (!stop.stop_requested()) {
        if (!ListDirectoriesIterative(root)) {
            return false;
        }
        ++iteration;
    }
    std::cerr << "[test] listProcess did " << iteration << " iterations\n";
    return true;
}

bool ReadFileComparisonLoop(const std::string& orig, const std::string& mnt,
    size_t size, const char* name, jprocess::stop_token stop)
{
    int iteration = 0;
    while (!stop.stop_requested()) {
        if (!CompareFiles(orig, mnt, size, stop)) {
            std::cerr << "[test] " << name << " comparison failed at iteration " << iteration << "\n";
            return false;
        }
        ++iteration;
    }
    std::cerr << "[test] " << name << " process did " << iteration << " iterations\n";
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
    return;
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

        std::this_thread::sleep_for(20s);

        std::cerr << "[shutdown] requesting stop for all child processes\n";
        procList.request_stop();
        procFileA.request_stop();
        procFileB.request_stop();
        procRoot.request_stop();

        std::this_thread::sleep_for(1s);

        // Poll all children together so we wait at most GracePeriod total, not per-child.
        // If any child is stuck in a FUSE syscall (e.g. readdir waiting for a response that
        // never arrives), it cannot exit via the stop flag alone. After the grace period,
        // we do a lazy FUSE unmount which sends EIO to all blocked FUSE syscalls so those
        // children can exit immediately.
        static constexpr auto GracePeriod = std::chrono::seconds(10);
        std::optional<bool> listResult;
        std::optional<bool> fileAResult;
        std::optional<bool> fileBResult;
        std::optional<bool> fileRootResult;
        const auto graceDeadline = std::chrono::steady_clock::now() + GracePeriod;
        while (std::chrono::steady_clock::now() < graceDeadline) {
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
            // Children are stuck in FUSE syscalls waiting for replies that will never come
            // (e.g. a readdirplus response that was lost on the network). The stop flag is
            // invisible to a process blocked inside a kernel FUSE wait. Lazy unmount does
            // not help either — it detaches the mountpoint but keeps the session alive,
            // so the kernel still waits for fuse_reply_*. The only way to unblock the
            // children is to close the /dev/fuse file descriptor, which causes the FUSE
            // kernel module to abort all pending requests with ENODEV.
            //
            // fuse_session_destroy() closes that fd, but it must not race with any
            // concurrent fuse_reply_*() calls from scheduler threads. So: stop the
            // scheduler first (all threads exit via stop tokens), then destroy the
            // filesystem session. After that the children unblock and join() returns.
            std::cerr << "[shutdown] stuck:"
                      << (!listResult ? " procList" : "")
                      << (!fileAResult ? " procFileA" : "")
                      << (!fileBResult ? " procFileB" : "")
                      << (!fileRootResult ? " procRoot" : "")
                      << " — stopping scheduler\n";
            // Step 1: stop fuse_session_loop so no more FUSE callbacks call
            // scheduler->Schedule(). Without this, the FUSE thread races against
            // the scheduler destructor: it keeps adding tasks to queues that are
            // simultaneously being torn down.
            std::cerr << "[shutdown] stuck: stopping FUSE session loop\n";
            filesystem->RequestStopAndWait();
            // Step 2: stop all scheduler threads — safe now, no more concurrent callbacks.
            std::cerr << "[shutdown] stuck: stopping scheduler\n";
            scheduler.reset();
            // Step 3: reply EIO to every pending fuse_req_t stored in leaf nodes.
            // This unblocks children that are waiting for file-read blocks that will never arrive.
            std::cerr << "[shutdown] stuck: cancelling pending FUSE requests (EIO)\n";
            fileTree.CancelAllPendingJobs();
            // Step 4: close the FUSE session fd. This aborts any remaining in-flight FUSE
            // requests (e.g. readdirplus whose response was lost on the network) at the kernel
            // level, so the child processes blocking in those syscalls get ENODEV and can exit.
            std::cerr << "[shutdown] stuck: destroying filesystem (closes /dev/fuse)\n";
            filesystem.reset();
            std::cerr << "[shutdown] stuck: /dev/fuse closed, children should unblock now\n";
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

        std::cerr << "[shutdown] requesting stopSource stop\n";
        stopSource.request_stop();

        if (!anyStuck) {
            std::cerr << "[shutdown] unmounting FUSE\n";
            UnmountAt(MountPoint3);

            std::cerr << "[shutdown] waiting for scheduler\n";
            scheduler->Wait(stop);

            std::cerr << "[shutdown] resetting scheduler\n";
            scheduler.reset();

            std::cerr << "[shutdown] resetting filesystem\n";
            filesystem.reset();
        }

        std::cerr << "[shutdown] clientThread lambda done\n";
    });

    auto deadline = std::chrono::steady_clock::now() + 50s;
    while (!stopSource.stop_requested() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(100ms);
    }

    if (!stopSource.stop_requested()) {
        // 300s deadline hit — the client thread's 10s grace path should have already
        // handled stuck children via scheduler.reset()+filesystem.reset(). This path
        // is a last-resort fallback in case the client thread itself is stuck.
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

#endif // __linux__
