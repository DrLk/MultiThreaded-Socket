#include "Socket.hpp"
#include "gtest/gtest.h"

#include <compare>
#include <vector>

#include "ConnectionAddr.hpp"

namespace FastTransport::Protocol {
TEST(SocketTest, Socket)
{
    Socket src(ConnectionAddr("0.0.0.0", 13100));
    Socket dst(ConnectionAddr("0.0.0.0", 13200));

    src.Init();
    dst.Init();

    constexpr int PacketSize = 1400;
    std::vector<unsigned char> data(PacketSize);
    for (int i = 0; i < PacketSize; ++i) {
        data[i] = static_cast<unsigned char>(i % 256);
    }

    const ConnectionAddr dstAddr("127.0.0.1", 13200);
    auto result = src.SendTo(data, dstAddr);
    EXPECT_EQ(result, PacketSize);

    ConnectionAddr addr;
    std::vector<unsigned char> data2(PacketSize);
    while (!dst.WaitRead()) { }
    result = dst.RecvFrom(data2, addr);
    EXPECT_EQ(result, PacketSize);

    EXPECT_TRUE(data <=> data2 == std::weak_ordering::equivalent);
}
} // namespace FastTransport::Protocol