#include "ConnectionReader.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <stop_token>

#include "ByteStream.hpp"
#include "ConnectionWriter.hpp"
#include "FastTransportProtocol.hpp"
#include "IPacket.hpp"
#include "Logger.hpp"
#include "Packet.hpp"
#include "UDPQueue.hpp"

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

TEST(ConnectionReader, Payload)
{
    const std::stop_source stopSource;
    const std::stop_token stop = stopSource.get_token();
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

TEST(ConnectionReaderIntegrated, ReadTrivial)
{
    static constexpr auto TestTimeout = 10s;

    std::packaged_task<void(std::stop_token stop)> sendTask([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", 11100));
        const ConnectionAddr dstAddr("127.0.0.1", 11200);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);
        IPacket::List sendPackets = UDPQueue::CreateBuffers(200000);
        srcConnection->AddFreeRecvPackets(std::move(recvPackets));
        srcConnection->AddFreeSendPackets(std::move(sendPackets));

        ConnectionWriter writer(stop, srcConnection);
        ConnectionReader reader(stop, srcConnection);
        FastTransport::FileSystem::OutputByteStream<ConnectionWriter> output(writer);
        FastTransport::FileSystem::InputByteStream<ConnectionReader> input(reader);

        const int aaa = 123;
        output << aaa;
        LOGGER() << "Write aaa: " << aaa;
        output.Flush();

        const int bbb = 124;
        output << bbb;
        LOGGER() << "Write bbb: " << bbb;
        output.Flush();

        uint64_t ccc = 0;
        input >> ccc;
        EXPECT_EQ(ccc, 321);
        LOGGER() << "Read ccc: " << ccc;
    });

    auto ready = sendTask.get_future();
    std::jthread sendThread(std::move(sendTask));

    std::jthread recvThread([&ready](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", 11200));

        const IConnection::Ptr dstConnection = dst.Accept(stop);
        if (dstConnection == nullptr) {
            throw std::runtime_error("Accept return nullptr");
        }

        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);
        IPacket::List sendPackets = UDPQueue::CreateBuffers(200000);
        dstConnection->AddFreeRecvPackets(std::move(recvPackets));
        dstConnection->AddFreeSendPackets(std::move(sendPackets));

        ConnectionWriter writer(stop, dstConnection);
        ConnectionReader reader(stop, dstConnection);
        FastTransport::FileSystem::OutputByteStream<ConnectionWriter> output(writer);
        FastTransport::FileSystem::InputByteStream<ConnectionReader> input(reader);

        int aaa = 0;
        input >> aaa;
        EXPECT_EQ(aaa, 123);
        LOGGER() << "Read aaa: " << aaa;

        int bbb = 0;
        input >> bbb;
        EXPECT_EQ(bbb, 124);
        LOGGER() << "Read bbb: " << bbb;

        const uint64_t ccc = 321;
        output << ccc;
        LOGGER() << "Write ccc: " << ccc;
        output.Flush();

        auto status = ready.wait_for(TestTimeout);

        EXPECT_TRUE(status == std::future_status::ready);
    });

    recvThread.join();
    sendThread.join();
}

} // namespace FastTransport::Protocol
