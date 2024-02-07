#include "ConnectionWriter.hpp"
#include <array>
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
    std::stop_source stopSource;
    std::stop_token stop = stopSource.get_token();
    auto connection = std::make_shared<MockConnection>();
    {
        IPacket::List packets;
        for (int i = 0; i < 100; i++) {
            auto packet = std::make_unique<Packet>(1500);
            std::array<std::byte, 1000> payload {};
            packet->SetPayload(payload);
            packets.push_back(std::move(packet));
        }

        ConnectionWriter writer(stop, connection, std::move(packets));

        IPacket::List list;
        /*EXPECT_CALL(*connection, Send(_, _))*/
        /* .WillOnce(DoAll(SaveArg<0>(&stop), SaveArg<1>(&list), Return(std::move(list)))); */
        /*.Times(2)
        .WillRepeatedly(
            [](std::stop_token stop, IPacket::List&& packets) {
                return std::move(packets);
            });*/

        /*EXPECT_CALL(*connection, Send)
            .WillOnce(
                [](std::stop_token stop, IPacket::List&& packets) {
                    return std::move(packets);
                });*/
        EXPECT_CALL(*connection, Send)
            .Times(4)
            .WillRepeatedly(
                [](std::stop_token stop, IPacket::List&& packets) {
                    return std::move(packets);
                });

        int value = 957;
        writer << value;

        writer.Flush();

        /*EXPECT_CALL(*connection, Send)
            .Times(2)
            .WillRepeatedly(
                [](std::stop_token stop, IPacket::List&& packets) {
                    return std::move(packets);
                });*/

        /* std::this_thread::sleep_for(std::chrono::seconds(1)); */
        writer << value;
        writer.Flush();
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        writer << value;
        writer.Flush();
        /* std::this_thread::sleep_for(std::chrono::seconds(1)); */
        int i = 0;
        i++;

        /*IPacket::List writePackets;

        writer.Write(std::move(writePackets));*/
    }
}
} // namespace FastTransport::Protocol
