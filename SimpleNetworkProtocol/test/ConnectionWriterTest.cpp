#include "ConnectionWriter.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <stop_token>
#include <vector>

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
            .Times(3)
            .WillRepeatedly(
                [](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(packets.size() == 1);
                    auto& packet = packets.back();
                    std::uint32_t size = 0;
                    std::memcpy(&size, packet->GetPayload().data(), sizeof(size));
                    EXPECT_TRUE(size == sizeof(int));
                    int value = 0;
                    std::memcpy(&value, packet->GetPayload().data() + sizeof(size), sizeof(value));
                    EXPECT_TRUE(value == 957);
                    return std::move(packets);
                });

        int value = 957;
        writer << value;
        writer.Flush();

        writer << value;
        writer.Flush();

        writer << value;
        writer.Flush();

        EXPECT_CALL(*connection, Send)
            .Times(3)
            .WillRepeatedly(
                [](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(packets.size() == 1);
                    auto& packet = packets.back();
                    std::uint32_t size = 0;
                    std::memcpy(&size, packet->GetPayload().data(), sizeof(size));
                    EXPECT_TRUE(size == sizeof(int));
                    int value = 0;
                    std::memcpy(&value, packet->GetPayload().data() + sizeof(size), sizeof(value));
                    EXPECT_TRUE(value == 958);
                    return std::move(packets);
                });
        value = 958;
        writer << value;
        writer.Flush();

        writer << value;
        writer.Flush();

        writer << value;
        writer.Flush();
    }
}

TEST(ConnectionWriter, EmptyFlush)
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

        EXPECT_CALL(*connection, Send)
            .Times(0);

        ConnectionWriter writer(stop, connection, std::move(packets));

        writer.Flush();
        writer.Flush();
        writer.Flush();
        writer.Flush();
        writer.Flush();
        writer.Flush();
    }
}

TEST(ConnectionWriter, WriteIPacketList)
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

        std::array<std::byte, 1000> testData {};
        for (auto& byte : testData) {
            byte = std::byte { 64 };
        }
        IPacket::List sendPackets;
        sendPackets.push_back(std::make_unique<Packet>(1200));
        sendPackets.back()->SetPayload(testData);

        EXPECT_TRUE(std::equal(testData.begin(), testData.end(), sendPackets.back()->GetPayload().begin(), sendPackets.back()->GetPayload().end()));

        EXPECT_CALL(*connection, Send)
            .WillOnce(
                [&testData](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(std::equal(testData.begin(), testData.end(), packets.back()->GetPayload().begin(), packets.back()->GetPayload().end()));
                    return std::move(packets);
                });

        writer << std::move(sendPackets);
        writer.Flush();
    }
}

TEST(ConnectionWriter, WriteBigArray)
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

        std::vector<std::byte> testData(2000, std::byte { 31 });

        EXPECT_CALL(*connection, Send)
            .WillOnce(
                [](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(packets.size() == 3);
                    auto& packet = packets.front();
                    std::uint32_t size = 0;
                    std::memcpy(&size, packet->GetPayload().data(), sizeof(size));
                    EXPECT_TRUE(size == 996);
                    /*int value = 0;
                    std::memcpy(&value, packet->GetPayload().data() + sizeof(size), sizeof(value));
                    EXPECT_TRUE(value == 958);*/
                    return std::move(packets);
                });

        writer.write(testData.data(), testData.size());
    }
}
} // namespace FastTransport::Protocol
