#include <gtest/gtest.h>

#include <chrono>

#include "Connection.hpp"
#include "ConnectionAddr.hpp"
#include "ConnectionState.hpp"
#include "IStatistics.hpp"
#include "Packet.hpp"

namespace FastTransport::Protocol {
TEST(StatisticaTest, BasicStatisticaTest)
{
    static constexpr auto TestTimeout = 10s;

    Connection connection(ConnectionState::DataState, ConnectionAddr("127.0.0.1", 10000), 1);
    const IStatistics& statistics = connection.GetStatistics();

    auto now = std::chrono::steady_clock::now();
    {
        IPacket::List dataPackets;
        dataPackets.push_back(std::make_unique<Packet>(1500));
        connection.SendDataPackets(std::move(dataPackets));
        auto packets = connection.GetPacketsToSend();
        while (packets.empty()) {
            if (now + TestTimeout < std::chrono::steady_clock::now()) {
                EXPECT_FALSE(packets.empty());
                break;
            }

            packets = connection.GetPacketsToSend();
        }
        EXPECT_EQ(statistics.GetSendPackets(), 1);
        EXPECT_EQ(packets.size(), 1);
    }

    {
        IPacket::List dataPackets;
        dataPackets.push_back(std::make_unique<Packet>(1500));
        connection.SendDataPackets(std::move(dataPackets));
        auto packets = connection.GetPacketsToSend();
        now = std::chrono::steady_clock::now();
        packets = connection.GetPacketsToSend();
        while (packets.empty()) {
            if (now + TestTimeout < std::chrono::steady_clock::now()) {
                EXPECT_FALSE(packets.empty());
                break;
            }

            packets = connection.GetPacketsToSend();
        }
        EXPECT_EQ(statistics.GetSendPackets(), 2);
        EXPECT_EQ(packets.size(), 1);
    }

    {
        IPacket::List servicePackets;
        servicePackets.push_back(std::make_unique<Packet>(1500));
        connection.SendServicePackets(std::move(servicePackets));
        EXPECT_EQ(statistics.GetAckSendPackets(), 1);
    }

    {
        IPacket::List servicePackets;
        servicePackets.push_back(std::make_unique<Packet>(1500));
        connection.SendServicePackets(std::move(servicePackets));
        EXPECT_EQ(statistics.GetAckSendPackets(), 2);
    }

    OutgoingPacket::List outgoingPackets;
    outgoingPackets.push_back(OutgoingPacket(std::make_unique<Packet>(1500), true));
    connection.ReSendPackets(std::move(outgoingPackets));
    EXPECT_EQ(statistics.GetLostPackets(), 1);

    OutgoingPacket::List outgoingPackets2;
    outgoingPackets2.push_back(OutgoingPacket(std::make_unique<Packet>(1500), true));
    outgoingPackets2.push_back(OutgoingPacket(std::make_unique<Packet>(1500), true));
    connection.ReSendPackets(std::move(outgoingPackets2));
    EXPECT_EQ(statistics.GetLostPackets(), 3);

    auto recvPacket = std::make_unique<Packet>(1500);
    recvPacket->SetSeqNumber(3);
    auto freePacket = connection.RecvPacket(std::move(recvPacket));
    EXPECT_TRUE(!freePacket);
    EXPECT_EQ(statistics.GetReceivedPackets(), 1);

    recvPacket = std::make_unique<Packet>(1500);
    recvPacket->SetSeqNumber(4);
    freePacket = connection.RecvPacket(std::move(recvPacket));
    EXPECT_TRUE(!freePacket);
    EXPECT_EQ(statistics.GetReceivedPackets(), 2);

    auto overflowPacket = std::make_unique<Packet>(1500);
    overflowPacket->SetSeqNumber(3 * 1024 * 1024);
    freePacket = connection.RecvPacket(std::move(overflowPacket));
    EXPECT_EQ(statistics.GetOverflowPackets(), 1);

    freePacket = connection.RecvPacket(std::move(freePacket));
    EXPECT_EQ(statistics.GetOverflowPackets(), 2);

    freePacket->SetSeqNumber(3);
    freePacket = connection.RecvPacket(std::move(freePacket));
    EXPECT_EQ(statistics.GetDuplicatePackets(), 1);

    freePacket->SetSeqNumber(4);
    freePacket = connection.RecvPacket(std::move(freePacket));
    EXPECT_EQ(statistics.GetDuplicatePackets(), 2);

    connection.AddAcks(std::span<SeqNumberType>());
    EXPECT_EQ(statistics.GetAckReceivedPackets(), 1);

    connection.AddAcks(std::span<SeqNumberType>());
    EXPECT_EQ(statistics.GetAckReceivedPackets(), 2);
}

} // namespace FastTransport::Protocol
