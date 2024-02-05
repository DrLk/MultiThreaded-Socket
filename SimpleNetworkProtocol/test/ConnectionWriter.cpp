#include "ConnectionWriter.hpp"
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

    MOCK_METHOD(IPacket::List, Send, (std::stop_token stop, IPacket::List&&));
    MOCK_METHOD(IPacket::List, Recv, (std::stop_token stop, IPacket::List&&));
    MOCK_METHOD(void, Close, ());
    MOCK_METHOD(bool, IsClosed, (), (const, override));
};

TEST(ConnectionWriter, Payload)
{
    MockConnection connection;
    IPacket::List packets;
    for (int i = 0; i < 100; i++) {
        packets.push_back(std::make_unique<Packet>(1500));
    }

    return;
    ConnectionWriter writer(std::unique_ptr<IConnection>(&connection), std::move(packets));

    EXPECT_CALL(connection, Send)
        .WillRepeatedly(
            [](std::stop_token /*stop*/, IPacket::List&& packets) {
                return std::move(packets);
            });
    int value = 957;
    writer << value;

    /*writer.Flush();

    IPacket::List writePackets;

    writer.Write(std::move(writePackets));*/
}
} // namespace FastTransport::Protocol
