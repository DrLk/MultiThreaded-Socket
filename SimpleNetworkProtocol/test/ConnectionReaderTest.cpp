#include "ConnectionReader.hpp"
#include <array>
#include <cstddef>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stop_token>

#include "IPacket.hpp"
#include "Packet.hpp"

namespace FastTransport::Protocol {
class MockConnection : public IConnection {
public:
    MockConnection() = default;

    MockConnection(const MockConnection&) = delete;
    MockConnection(MockConnection&&) = delete;
    MockConnection& operator=(const MockConnection&) = delete;
    MockConnection& operator=(MockConnection&&) = delete;
    ~MockConnection() override = default;

    MOCK_METHOD(bool, IsConnected, (), (const, override));
    MOCK_METHOD(IStatistics&, GetStatistics, (), (const, override));
    MOCK_METHOD(ConnectionContext&, GetContext, ());

    MOCK_METHOD(IPacket::List, Send, (std::stop_token, IPacket::List&&));
    MOCK_METHOD(IPacket::List, Recv, (std::stop_token, IPacket::List&&));
    MOCK_METHOD(IPacket::List, Recv, (std::size_t, std::stop_token, IPacket::List&&));
    MOCK_METHOD(void, AddFreeRecvPackets, (IPacket::List&&));
    MOCK_METHOD(void, AddFreeSendPackets, (IPacket::List&&));
    MOCK_METHOD(void, Close, ());
    MOCK_METHOD(bool, IsClosed, (), (const, override));
};

TEST(ConnectionReader, Payload)
{
    std::stop_source stopSource;
    std::stop_token stop = stopSource.get_token();
    auto connection = std::make_shared<MockConnection>();
    {
        std::array<std::byte, 1000> testData {};
        EXPECT_CALL(*connection, Recv(testing::_, testing::_))
            .WillOnce(
                [&testData](std::stop_token /*stop*/, IPacket::List&& freePackets) {
                    EXPECT_TRUE(freePackets.empty());
                    IPacket::Ptr packet = std::make_unique<Packet>(1500);
                    packet->SetPayload(testData);
                    IPacket::List packets = std::move(freePackets);
                    packets.push_back(std::move(packet));
                    return packets;
                });

        ConnectionReader reader(stop, connection);

        std::array<std::byte, 1000> payload {};
        reader.read(payload.data(), payload.size());
        EXPECT_THAT(payload, ::testing::ElementsAreArray(testData));
    }
}

} // namespace FastTransport::Protocol
