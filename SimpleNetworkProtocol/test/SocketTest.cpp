#include "Socket.hpp"
#include <gtest/gtest.h>

#include <compare>
#include <cstddef>
#include <vector>

#include "ConnectionAddr.hpp"
#include "IPacket.hpp"
#include "Logger.hpp"
#include "Packet.hpp"

namespace FastTransport::Protocol {
TEST(SocketTest, Socket)
{
    Socket src(ConnectionAddr("0.0.0.0", 13100));
    Socket dst(ConnectionAddr("0.0.0.0", 13200));

    src.Init();
    dst.Init();

    constexpr int PacketSize = 1400;
    std::vector<std::byte> data(PacketSize);
    for (int i = 0; i < PacketSize; ++i) {
        data[i] = static_cast<std::byte>(i % 256);
    }

    const ConnectionAddr dstAddr("127.0.0.1", 13200);
    auto result = src.SendTo(data, dstAddr);
    EXPECT_EQ(result, PacketSize);

    ConnectionAddr addr;
    std::vector<std::byte> data2(PacketSize);
    while (!dst.WaitRead()) { }
    result = dst.RecvFrom(data2, addr);
    EXPECT_EQ(result, PacketSize);

    EXPECT_TRUE(data <=> data2 == std::weak_ordering::equivalent);
}


#ifdef __linux__

TEST(SocketTest, GSOSendMsg)
{
    /* Socket src(ConnectionAddr("0.0.0.0", 13100)); */
    /* Socket dst(ConnectionAddr("0.0.0.0", 13200)); */
    Socket src(ConnectionAddr("192.168.100.12", 13100));
    Socket dst1(ConnectionAddr("192.168.100.12", 13201));
    Socket dst2(ConnectionAddr("192.168.100.12", 13202));

    src.Init();
    dst1.Init();
    dst2.Init();

    constexpr int PacketSize = Socket::GsoSize;
    std::vector<std::byte> data(PacketSize);
    for (int i = 0; i < PacketSize; ++i) {
        data[i] = static_cast<std::byte>(i % 256);
    }

    /* const ConnectionAddr dstAddr("127.0.0.1", 13200); */
    const ConnectionAddr dstAddr1("192.168.100.12", 13201);
    const ConnectionAddr dstAddr2("192.168.100.12", 13202);
    IPacket::List sendPackets1;
    IPacket::List sendPackets2;
    std::array<std::byte, PacketSize - 100> payload {};

    static constexpr int PacketNumber = Socket::UDPMaxSegments;
    for (int i = 0; i < PacketNumber; ++i) {
        auto packet = std::make_unique<Packet>(PacketSize);
        packet->SetPayload(payload);
        packet->SetAddr(dstAddr1);
        sendPackets1.push_back(std::move(packet));

        packet = std::make_unique<Packet>(PacketSize);
        packet->SetPayload(payload);
        packet->SetAddr(dstAddr2);
        sendPackets1.push_back(std::move(packet));
    }

    for (int i = 0; i < PacketNumber / 2; ++i) {
        auto packet = std::make_unique<Packet>(PacketSize);
        packet->SetPayload(payload);
        packet->SetAddr(dstAddr2);
        sendPackets2.push_back(std::move(packet));
    }

    for (int i = 0; i < 3; ++i) {
        auto result = src.SendMsg(sendPackets1);
        result = src.SendMsg(sendPackets2);
        EXPECT_EQ(result, PacketSize);
    }

    IPacket::List recvPackets2 {};
    for (int i = 0; i < 128; ++i) {
        auto packet = std::make_unique<Packet>(1400);
        recvPackets2.push_back(std::move(packet));
    }

    /* while (!dst1.WaitRead()) { } */
    /* auto result = dst1.RecvMsg(recvPackets2, 0); */
    /* recvPackets2.splice(std::move(result)); */
    /* while (!dst1.WaitRead()) { } */
    /* result = dst1.RecvMsg(recvPackets2, 0); */
    /* recvPackets2.splice(std::move(result)); */
    /* while (!dst1.WaitRead()) { } */
    /* result = dst1.RecvMsg(recvPackets2, 0); */
    /* recvPackets2.splice(std::move(result)); */
    /* while (!dst1.WaitRead()) { } */
    /* result = dst1.RecvMsg(recvPackets2, 0); */
    /* recvPackets2.splice(std::move(result)); */
    /* while (!dst2.WaitRead()) { } */
    /* result = dst2.RecvMsg(recvPackets2, 0); */
    /* recvPackets2.splice(std::move(result)); */
    /* while (!dst1.WaitRead()) { } */
    /* result = dst1.RecvMsg(recvPackets2, 0); */
    /* recvPackets2.splice(std::move(result)); */
    /* while (!dst2.WaitRead()) { } */
    /* result = dst2.RecvMsg(recvPackets2, 0); */
    /* recvPackets2.splice(std::move(result)); */

    /* ConnectionAddr addr; */
    /* std::vector<std::byte> data2(PacketSize); */
    /* while (!dst.WaitRead()) { } */
    /* result = dst.RecvFrom(data2, addr); */
    /* while (!dst.WaitRead()) { } */
    /* result = dst.RecvFrom(data2, addr); */
    /* while (!dst.WaitRead()) { } */
    /* result = dst.RecvFrom(data2, addr); */
    /* while (!dst.WaitRead()) { } */
    /* result = dst.RecvFrom(data2, addr); */
    /* EXPECT_EQ(result, PacketSize); */
    /*  */
    /* EXPECT_TRUE(data <=> data2 == std::weak_ordering::equivalent); */

    auto now = std::chrono::high_resolution_clock::now();
    size_t packetsPerSecond = 0;
    while (true) {
        auto result = src.SendMsg(sendPackets1);
        packetsPerSecond += result;
        if (now + std::chrono::seconds(1) < std::chrono::high_resolution_clock::now()) {
            now = std::chrono::high_resolution_clock::now();
            LOGGER() << "Send packets: " << packetsPerSecond;
        }

        if (packetsPerSecond >= static_cast<int64_t>(10) * 1024 * 1024) {
            LOGGER() << "Send packets: " << packetsPerSecond;
            break;
        }
    }

    IPacket::List recvPackets {};
    for (int i = 0; i < 16; ++i) {
        auto packet = std::make_unique<Packet>(1024 * 1024);
        recvPackets.push_back(std::move(packet));
    }

    /* packetsPerSecond = 0; */
    /* while (true) { */
    /*     auto result = dst1.RecvMsg(recvPackets, 0); */
    /*     packetsPerSecond += recvPackets.size(); */
    /*     if (now + std::chrono::seconds(1) < std::chrono::high_resolution_clock::now()) { */
    /*         now = std::chrono::high_resolution_clock::now(); */
    /*         LOGGER() << "Recv packets: " << packetsPerSecond; */
    /*         packetsPerSecond = 0; */
    /*     } */
    /* } */
}

#endif

} // namespace FastTransport::Protocol
