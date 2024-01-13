#include "Socket.hpp"
#include <gtest/gtest.h>

#include <chrono>
#include <compare>
#include <cstddef>
#include <vector>

#include "ConnectionAddr.hpp"
#include "IPacket.hpp"
#include "OutgoingPacket.hpp"
#include "Packet.hpp"

namespace FastTransport::Protocol {

using namespace std::chrono_literals;

TEST(SocketTest, Socket)
{
    Socket src(ConnectionAddr("0.0.0.0", 13100));
    Socket dst(ConnectionAddr("0.0.0.0", 13200));

    src.Init();
    dst.Init();

    constexpr int PacketSize = Socket::GsoSize;
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
    static constexpr auto TestTimeout = 10s;

    Socket src(ConnectionAddr("0.0.0.0", 13100));
    Socket dst1(ConnectionAddr("0.0.0.0", 13201));
    Socket dst2(ConnectionAddr("0.0.0.0", 13202));

    src.Init();
    dst1.Init();
    dst2.Init();

    constexpr int PacketSize = Socket::GsoSize;
    std::vector<std::byte> data(PacketSize);
    for (int i = 0; i < PacketSize; ++i) {
        data[i] = static_cast<std::byte>(i % 256);
    }

    const ConnectionAddr dstAddr1("127.0.0.1", 13201);
    const ConnectionAddr dstAddr2("127.0.0.1", 13202);
    OutgoingPacket::List sendPackets1;
    OutgoingPacket::List sendPackets2;
    std::array<std::byte, PacketSize - 100> payload {};

    static constexpr int PacketNumber = Socket::UDPMaxSegments;
    for (int i = 0; i < PacketNumber; ++i) {
        auto packet = std::make_unique<Packet>(PacketSize);
        packet->SetPayload(payload);
        packet->SetAddr(dstAddr1);
        sendPackets1.push_back({ std::move(packet), false });

        packet = std::make_unique<Packet>(PacketSize);
        packet->SetPayload(payload);
        packet->SetAddr(dstAddr2);
        sendPackets1.push_back({ std::move(packet), false });
    }

    auto result = src.SendMsg(sendPackets1, 0);
    EXPECT_EQ(result, PacketNumber * 2 * PacketSize);

    IPacket::List recvPackets {};
    for (int i = 0; i < PacketNumber; ++i) {
        auto packet = std::make_unique<Packet>(PacketSize);
        recvPackets.push_back(std::move(packet));
    }

    auto now = std::chrono::steady_clock::now();
    size_t recvSize = 0;
    while (recvSize < PacketNumber) {
        if (now + TestTimeout < std::chrono::steady_clock::now()) {
            EXPECT_EQ(recvSize, PacketNumber);
            break;
        }

        if (dst1.WaitRead()) {
            auto freePackets = dst1.RecvMsg(recvPackets, 0);
            recvSize += recvPackets.size();
            recvPackets.splice(std::move(freePackets));
        }
    }
    EXPECT_EQ(recvSize, PacketNumber);

    now = std::chrono::steady_clock::now();
    recvSize = 0;
    while (recvSize < PacketNumber) {
        if (now + TestTimeout < std::chrono::steady_clock::now()) {
            EXPECT_EQ(recvSize, PacketNumber);
            break;
        }

        if (dst2.WaitRead()) {
            auto freePackets = dst2.RecvMsg(recvPackets, 0);
            recvSize += recvPackets.size();
            recvPackets.splice(std::move(freePackets));
        }
    }
    EXPECT_EQ(recvSize, PacketNumber);
}

#endif

} // namespace FastTransport::Protocol
