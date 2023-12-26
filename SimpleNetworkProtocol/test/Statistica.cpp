#include "gtest/gtest.h"

#include "Connection.hpp"
#include "ConnectionAddr.hpp"
#include "ConnectionState.hpp"
#include "IStatistica.hpp"
#include "Packet.hpp"

namespace FastTransport::Protocol {
TEST(StatisticaTest, BasicStatisticaTest)
{
    /* Connection(ConnectionState state, const ConnectionAddr& addr, ConnectionID myID); */
    Connection connection(ConnectionState::DataState, ConnectionAddr("127.0.0.1", 10000), 1);
    IStatistica& statistica = connection.GetStatistica();

    connection.SendPacket(std::make_unique<Packet>(1500), true);
    EXPECT_EQ(statistica.GetSendPackets(), 1);

    connection.SendPacket(std::make_unique<Packet>(1500), true);
    EXPECT_EQ(statistica.GetSendPackets(), 2);

    connection.SendPacket(std::make_unique<Packet>(1500), false);
    EXPECT_EQ(statistica.GetAckSendPackets(), 1);

    connection.SendPacket(std::make_unique<Packet>(1500), false);
    EXPECT_EQ(statistica.GetAckSendPackets(), 2);

    OutgoingPacket::List outgoingPackets;;
    outgoingPackets.push_back(OutgoingPacket(std::make_unique<Packet>(1500), true));
    connection.ReSendPackets(std::move(outgoingPackets));
    EXPECT_EQ(statistica.GetLostPackets(), 1);

    OutgoingPacket::List outgoingPackets2;
    outgoingPackets2.push_back(OutgoingPacket(std::make_unique<Packet>(1500), true));
    outgoingPackets2.push_back(OutgoingPacket(std::make_unique<Packet>(1500), true));
    connection.ReSendPackets(std::move(outgoingPackets2));
    EXPECT_EQ(statistica.GetLostPackets(), 3);

    auto recvPacket = std::make_unique<Packet>(1500);
    recvPacket->SetSeqNumber(3);
    auto freePacket = connection.RecvPacket(std::move(recvPacket));
    EXPECT_TRUE(!freePacket);
    EXPECT_EQ(statistica.GetReceivedPackets(), 1);

    recvPacket = std::make_unique<Packet>(1500);
    recvPacket->SetSeqNumber(4);
    freePacket = connection.RecvPacket(std::move(recvPacket));
    EXPECT_TRUE(!freePacket);
    EXPECT_EQ(statistica.GetReceivedPackets(), 2);

    auto overflowPacket = std::make_unique<Packet>(1500);
    overflowPacket->SetSeqNumber(3 * 1024 * 1024);
    freePacket = connection.RecvPacket(std::move(overflowPacket));
    EXPECT_EQ(statistica.GetOverflowPackets(), 1);

    freePacket = connection.RecvPacket(std::move(freePacket));
    EXPECT_EQ(statistica.GetOverflowPackets(), 2);

    freePacket->SetSeqNumber(3);
    freePacket = connection.RecvPacket(std::move(freePacket));
    EXPECT_EQ(statistica.GetDuplicatePackets(), 1);

    freePacket->SetSeqNumber(4);
    freePacket = connection.RecvPacket(std::move(freePacket));
    EXPECT_EQ(statistica.GetDuplicatePackets(), 2);

    connection.AddAcks(std::span<SeqNumberType>());
    EXPECT_EQ(statistica.GetAckReceivedPackets(), 1);

    connection.AddAcks(std::span<SeqNumberType>());
    EXPECT_EQ(statistica.GetAckReceivedPackets(), 2);
}

} // namespace FastTransport::Protocol
