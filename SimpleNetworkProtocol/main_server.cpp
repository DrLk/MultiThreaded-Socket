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

#include "FileTree.hpp"
#include "InotifyWatcherJob.hpp"
#include "ServerMessageTypeReadJob.hpp"
#include "ServerTaskScheduler.hpp"

namespace {

using FastTransport::FileSystem::FileTree;
using FastTransport::Protocol::ConnectionAddr;
using FastTransport::Protocol::FastTransportContext;
using FastTransport::Protocol::IConnection;
using FastTransport::Protocol::IPacket;
using FastTransport::Protocol::UDPQueue;
using FastTransport::TaskQueue::ServerMessageTypeReadJob;
using FastTransport::TaskQueue::ServerTaskScheduler;

void RunFileServer(std::stop_token stop, std::string_view srcAddress, uint16_t srcPort, std::string_view dstAddress, uint16_t dstPort, std::string_view shareDir, std::string_view cacheDir)
{
    FastTransportContext src(ConnectionAddr(srcAddress, srcPort));
    const ConnectionAddr dstAddr(dstAddress, dstPort);

    const IConnection::Ptr srcConnection = src.Connect(dstAddr);

    IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);
    IPacket::List sendPackets = UDPQueue::CreateBuffers(200000);
    srcConnection->AddFreeRecvPackets(std::move(recvPackets));
    srcConnection->AddFreeSendPackets(std::move(sendPackets));

    std::filesystem::path shareP(shareDir);
    std::filesystem::path cacheP(cacheDir);
    if (!std::filesystem::exists(cacheP)) {
        std::filesystem::create_directories(cacheP);
    }

    const std::filesystem::path watchRoot = shareP;
    FileTree fileTree(std::move(shareP), std::move(cacheP));
    ServerTaskScheduler scheduler(*srcConnection, fileTree);
    scheduler.Schedule(ServerMessageTypeReadJob::Create(fileTree, IPacket::List()));
    scheduler.Schedule(std::make_unique<FastTransport::TaskQueue::InotifyWatcherJob>(watchRoot, fileTree, scheduler));
    scheduler.Wait(stop);
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
    // args: <bind_addr> <bind_port> <dst_addr> <dst_port> <share_dir> <cache_dir>
    if (argc < 7) {
        std::cerr << "Usage: " << (argc > 0 ? argv[0] : "SimpleNetworkProtocolServer")
                  << " <bind_addr> <bind_port> <dst_addr> <dst_port> <share_dir> <cache_dir>\n";
        return 1;
    }
    auto args = std::span(argv, argc);
    const std::string_view srcAddress = args[1];
    const uint16_t srcPort = ParsePort(args[2]);
    const std::string_view dstAddress = args[3];
    const uint16_t dstPort = ParsePort(args[4]);
    const std::string_view shareDir = args[5];
    const std::string_view cacheDir = args[6];
    RunFileServer(std::stop_token {}, srcAddress, srcPort, dstAddress, dstPort, shareDir, cacheDir);
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
