#include "ConnectionWriter.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
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

    MOCK_METHOD(IPacket::List, GetFreeSendPackets, (std::stop_token));
    MOCK_METHOD(void, Send, (IPacket::List&&));
    MOCK_METHOD(IPacket::List, Send2, (std::stop_token, IPacket::List&&));
    MOCK_METHOD(IPacket::List, Recv, (std::stop_token, IPacket::List&&));
    MOCK_METHOD(IPacket::List, Recv, (std::size_t, std::stop_token, IPacket::List&&));
    MOCK_METHOD(void, AddFreeRecvPackets, (IPacket::List&&));
    MOCK_METHOD(void, AddFreeSendPackets, (IPacket::List&&));
    MOCK_METHOD(void, Close, ());
    MOCK_METHOD(bool, IsClosed, (), (const, override));
};

TEST(ConnectionWriter, Payload)
{
    const std::stop_source stopSource;
    const std::stop_token stop = stopSource.get_token();
    auto connection = std::make_shared<MockConnection>();
    {
        EXPECT_CALL(*connection, Send2)
            .WillOnce(
                [](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(packets.empty());
                    IPacket::List freePackets = std::move(packets);
                    for (int i = 0; i < 100; i++) {
                        auto packet = std::make_unique<Packet>(1500);
                        std::array<std::byte, 1000> payload {};
                        packet->SetPayload(payload);
                        freePackets.push_back(std::move(packet));
                    }
                    return freePackets;
                });

        ConnectionWriter writer(stop, connection);

        EXPECT_CALL(*connection, Send2)
            .Times(3)
            .WillRepeatedly(
                [](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(packets.size() == 1);
                    auto& packet = packets.back();
                    int value = 0;
                    std::memcpy(&value, packet->GetPayload().data(), sizeof(value));
                    EXPECT_TRUE(value == 957);
                    return std::move(packets);
                });

        int value = 957;
        writer << value;
        writer.flush();

        writer << value;
        writer.flush();

        writer << value;
        writer.flush();

        EXPECT_CALL(*connection, Send2)
            .Times(3)
            .WillRepeatedly(
                [](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(packets.size() == 1);
                    auto& packet = packets.back();
                    EXPECT_EQ(packet->GetPayload().size(), sizeof(value));
                    int value = 0;
                    std::memcpy(&value, packet->GetPayload().data(), sizeof(value));
                    EXPECT_TRUE(value == 958);
                    return std::move(packets);
                });
        value = 958;
        writer << value;
        writer.flush();

        writer << value;
        writer.flush();

        writer << value;
        writer.flush();
    }
}

TEST(ConnectionWriter, EmptyFlush)
{
    const std::stop_source stopSource;
    const std::stop_token stop = stopSource.get_token();
    auto connection = std::make_shared<MockConnection>();
    {
        EXPECT_CALL(*connection, Send2)
            .WillOnce(
                [](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(packets.empty());
                    IPacket::List freePackets = std::move(packets);
                    for (int i = 0; i < 100; i++) {
                        auto packet = std::make_unique<Packet>(1500);
                        std::array<std::byte, 1000> payload {};
                        packet->SetPayload(payload);
                        freePackets.push_back(std::move(packet));
                    }
                    return freePackets;
                });

        ConnectionWriter writer(stop, connection);

        EXPECT_CALL(*connection, Send2)
            .Times(0);
        writer.flush();
        writer.flush();
        writer.flush();
        writer.flush();
        writer.flush();
        writer.flush();
    }
}

TEST(ConnectionWriter, WriteIPacketList)
{
    const std::stop_source stopSource;
    const std::stop_token stop = stopSource.get_token();
    auto connection = std::make_shared<MockConnection>();
    {
        EXPECT_CALL(*connection, Send2)
            .WillOnce(
                [](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(packets.empty());
                    IPacket::List freePackets = std::move(packets);
                    for (int i = 0; i < 100; i++) {
                        auto packet = std::make_unique<Packet>(1500);
                        std::array<std::byte, 1000> payload {};
                        packet->SetPayload(payload);
                        freePackets.push_back(std::move(packet));
                    }
                    return freePackets;
                });

        ConnectionWriter writer(stop, connection);

        std::array<std::byte, 1000> testData {};
        for (auto& byte : testData) {
            byte = std::byte { 64 };
        }
        IPacket::List sendPackets;
        sendPackets.push_back(std::make_unique<Packet>(1200));
        sendPackets.back()->SetPayload(testData);

        EXPECT_TRUE(std::equal(testData.begin(), testData.end(), sendPackets.back()->GetPayload().begin(), sendPackets.back()->GetPayload().end()));

        EXPECT_CALL(*connection, Send2)
            .WillOnce(
                [&testData](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(std::equal(testData.begin(), testData.end(), packets.back()->GetPayload().begin(), packets.back()->GetPayload().end()));
                    return std::move(packets);
                });

        writer << std::move(sendPackets);
        writer.flush();
    }
}

TEST(ConnectionWriter, WriteBigArray)
{
    const std::stop_source stopSource;
    const std::stop_token stop = stopSource.get_token();
    auto connection = std::make_shared<MockConnection>();
    {
        EXPECT_CALL(*connection, Send2)
            .WillOnce(
                [](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(packets.empty());
                    IPacket::List freePackets = std::move(packets);
                    for (int i = 0; i < 100; i++) {
                        auto packet = std::make_unique<Packet>(1500);
                        std::array<std::byte, 1000> payload {};
                        packet->SetPayload(payload);
                        freePackets.push_back(std::move(packet));
                    }
                    return freePackets;
                });

        ConnectionWriter writer(stop, connection);

        std::vector<std::byte> testData(2000, std::byte { 31 });

        EXPECT_CALL(*connection, Send2)
            .WillOnce(
                [](std::stop_token /*stop*/, IPacket::List&& packets) {
                    EXPECT_TRUE(packets.size() == 2);
                    return std::move(packets);
                });

        writer.write(testData.data(), testData.size());
    }
}
} // namespace FastTransport::Protocol
