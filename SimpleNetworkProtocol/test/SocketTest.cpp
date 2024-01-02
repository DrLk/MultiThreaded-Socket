#include "Socket.hpp"
#include <gtest/gtest.h>

#include <compare>
#include <cstddef>
#include <vector>

#include "ConnectionAddr.hpp"
#include "IPacket.hpp"
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
    Socket src(ConnectionAddr("0.0.0.0", 13100));
    Socket dst(ConnectionAddr("0.0.0.0", 13200));

    src.Init();
    dst.Init();

    constexpr int PacketSize = 3400;
    std::vector<std::byte> data(PacketSize);
    for (int i = 0; i < PacketSize; ++i) {
        data[i] = static_cast<std::byte>(i % 256);
    }

    const ConnectionAddr dstAddr("127.0.0.1", 13200);
    IPacket::List packets;
    auto packet = std::make_unique<Packet>(PacketSize);
    std::array<std::byte, PacketSize - 100> payload {};
    packet->SetPayload(payload);
    packets.push_back(std::move(packet));
    auto result = src.SendMsg(packets, dstAddr);
    EXPECT_EQ(result, PacketSize);

    ConnectionAddr addr;
    std::vector<std::byte> data2(PacketSize);
    while (!dst.WaitRead()) { }
    result = dst.RecvFrom(data2, addr);
    EXPECT_EQ(result, PacketSize);

    EXPECT_TRUE(data <=> data2 == std::weak_ordering::equivalent);
}
} // namespace FastTransport::Protocol
