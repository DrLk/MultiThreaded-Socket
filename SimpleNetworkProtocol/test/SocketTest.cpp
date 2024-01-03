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

TEST(SocketTest, GSOSendMsg)
{
    /* Socket src(ConnectionAddr("0.0.0.0", 13100)); */
    /* Socket dst(ConnectionAddr("0.0.0.0", 13200)); */
    Socket src(ConnectionAddr("192.168.100.12", 13100));
    Socket dst(ConnectionAddr("192.168.100.12", 13200));

    src.Init();
    dst.Init();

    constexpr int PacketSize = 200;
    std::vector<std::byte> data(PacketSize);
    for (int i = 0; i < PacketSize; ++i) {
        data[i] = static_cast<std::byte>(i % 256);
    }

    /* const ConnectionAddr dstAddr("127.0.0.1", 13200); */
    const ConnectionAddr dstAddr("192.168.100.12", 13200);
    IPacket::List sendPackets;
    std::array<std::byte, PacketSize - 100> payload {};

    static constexpr int PacketNumber = 2;
    for (int i = 0; i < PacketNumber; ++i) {
        auto packet = std::make_unique<Packet>(PacketSize);
        packet->SetPayload(payload);
        packet->SetAddr(dstAddr);
        sendPackets.push_back(std::move(packet));
    }

    for (int i = 0; i < 3; ++i) {
        auto result = src.SendMsg(sendPackets, dstAddr);
        EXPECT_EQ(result, PacketSize);
    }

    IPacket::List recvPackets2 {};
    for (int i = 0; i < 10; ++i) {
        auto packet = std::make_unique<Packet>(400);
        recvPackets2.push_back(std::move(packet));
    }

    ConnectionAddr srcAddr;
    while (!dst.WaitRead()) { }
    auto result = dst.RecvMsg(recvPackets2, srcAddr);
    while (!dst.WaitRead()) { }
    result = dst.RecvMsg(recvPackets2, srcAddr);
    while (!dst.WaitRead()) { }
    result = dst.RecvMsg(recvPackets2, srcAddr);
    while (!dst.WaitRead()) { }
    result = dst.RecvMsg(recvPackets2, srcAddr);

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
        result = src.SendMsg(sendPackets, dstAddr);
        packetsPerSecond += result;
        if (now + std::chrono::seconds(1) < std::chrono::high_resolution_clock::now()) {
            now = std::chrono::high_resolution_clock::now();
            LOGGER() << "Send packets: " << packetsPerSecond;
        }

        if (packetsPerSecond >= 10 * 1024 * 1024) {
            LOGGER() << "Send packets: " << packetsPerSecond;
            break;
        }
    }

    IPacket::List recvPackets {};
    for (int i = 0; i < 16; ++i) {
        auto packet = std::make_unique<Packet>(1024 * 1024);
        recvPackets.push_back(std::move(packet));
    }

    packetsPerSecond = 0;
    while (true) {
        ConnectionAddr srcAddr;
        result = dst.RecvMsg(recvPackets, srcAddr);
        packetsPerSecond += result;
        if (now + std::chrono::seconds(1) < std::chrono::high_resolution_clock::now()) {
            now = std::chrono::high_resolution_clock::now();
            LOGGER() << "Recv packets: " << packetsPerSecond;
            packetsPerSecond = 0;
        }
    }
}
} // namespace FastTransport::Protocol
