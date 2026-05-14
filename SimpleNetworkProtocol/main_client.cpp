#include <cstdint>
#include <filesystem>
#include <iostream>
#include <span>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>

#include "ConnectionAddr.hpp"
#include "FastTransportProtocol.hpp"
#include "IPacket.hpp"
#include "UDPQueue.hpp"

#include "ClientMessageTypeReadJob.hpp"
#include "ClientTaskScheduler.hpp"
#include "FileSystem/RemoteFileSystem.hpp"
#include "FileTree.hpp"

namespace {

using FastTransport::FileSystem::FileTree;
using FastTransport::Protocol::ConnectionAddr;
using FastTransport::Protocol::FastTransportContext;
using FastTransport::Protocol::IConnection;
using FastTransport::Protocol::IPacket;
using FastTransport::Protocol::UDPQueue;
using FastTransport::TaskQueue::ClientMessageTypeReadJob;
using FastTransport::TaskQueue::ClientTaskScheduler;
using FastTransport::TaskQueue::RemoteFileSystem;

void RunFuseClient(std::stop_token stop, std::string_view bindAddress, uint16_t bindPort, std::string_view mountPoint, std::string_view cacheDir)
{
    FastTransportContext dst(ConnectionAddr(bindAddress, bindPort));

    const IConnection::Ptr dstConnection = dst.Accept(stop);
    if (dstConnection == nullptr) {
        throw std::runtime_error("Accept returned nullptr");
    }

    IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);
    IPacket::List sendPackets = UDPQueue::CreateBuffers(200000);
    dstConnection->AddFreeRecvPackets(std::move(recvPackets));
    dstConnection->AddFreeSendPackets(std::move(sendPackets));

    std::filesystem::path cacheRoot { cacheDir };
    std::filesystem::path cacheFolder { cacheDir };
    if (!std::filesystem::exists(cacheFolder)) {
        std::filesystem::create_directories(cacheFolder);
    }

    FileTree fileTree(std::move(cacheRoot), std::move(cacheFolder));
    ClientTaskScheduler destinationScheduler(*dstConnection, fileTree);
    destinationScheduler.Schedule(ClientMessageTypeReadJob::Create(fileTree, IPacket::List()));

    RemoteFileSystem filesystem(mountPoint);
    RemoteFileSystem::scheduler = &destinationScheduler;
    filesystem.Init();
    RemoteFileSystem::session = filesystem.GetSession();
    filesystem.Start(stop);

    destinationScheduler.Wait(stop);
}

uint16_t ParsePort(std::string_view arg)
{
    const int value = std::stoi(std::string(arg));
    if (value < 0 || value > UINT16_MAX) {
        throw std::out_of_range("port out of range: " + std::string(arg));
    }
    return static_cast<uint16_t>(value);
}

} // namespace

int main(int argc, char** argv)
try {
    const auto args = std::span(argv, static_cast<std::size_t>(argc));
    // args: <bind_addr> <bind_port> <mount_point> <cache_dir>
    if (args.size() < 5) {
        const std::string_view progName = args.empty() ? "SimpleNetworkProtocolClient" : args.front();
        std::cerr << "Usage: " << progName
                  << " <bind_addr> <bind_port> <mount_point> <cache_dir>\n";
        return 1;
    }
    const std::string_view bindAddress = args[1];
    const uint16_t bindPort = ParsePort(args[2]);
    const std::string_view mountPoint = args[3];
    const std::string_view cacheDir = args[4];
    RunFuseClient(std::stop_token {}, bindAddress, bindPort, mountPoint, cacheDir);
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
