#include "IConnectionState.hpp"
#include <gtest/gtest.h>

#include "Connection.hpp"
#include "ConnectionAddr.hpp"
#include "HeaderTypes.hpp"
#include "UDPQueue.hpp"

namespace FastTransport::Protocol {

namespace {
    constexpr auto TestTimeout = 10s;

    std::unordered_map<ConnectionState, std::unique_ptr<IConnectionState>> GetStates()
    {
        std::unordered_map<ConnectionState, std::unique_ptr<IConnectionState>> states;
        states.emplace(ConnectionState::ClosedState, std::make_unique<ClosedState>());
        states.emplace(ConnectionState::ClosingState, std::make_unique<ClosingState>());
        states.emplace(ConnectionState::DataState, std::make_unique<DataState>());
        states.emplace(ConnectionState::SendingSynAckState, std::make_unique<SendingSynAckState>());
        states.emplace(ConnectionState::SendingSynState, std::make_unique<SendingSynState>());
        states.emplace(ConnectionState::WaitingSynAckState, std::make_unique<WaitingSynAckState>());
        states.emplace(ConnectionState::WaitingSynState, std::make_unique<WaitingSynState>());
        return states;
    }

    OutgoingPacket::List GetSendPackets(Connection& connection)
    {
        auto timeout = std::chrono::steady_clock::now() + TestTimeout;
        OutgoingPacket::List packets;
        while (timeout > std::chrono::steady_clock::now()) {
            packets = connection.GetPacketsToSend();
            if (!packets.empty()) {
                break;
            }

            std::this_thread::sleep_for(100ms);
        }

        EXPECT_TRUE(timeout > std::chrono::steady_clock::now()) << "Timeout waiting for send packets";

        return packets;
    }
} // namespace

TEST(ConnectionStateTest, ConnectionState)
{
    auto states = GetStates();

    const ConnectionAddr destinationAddress;
    const ConnectionID sourceID = 1;
    auto sourceState = ConnectionState::SendingSynState;
    Connection connection(sourceState, destinationAddress, sourceID);
    connection.SetInternalFreePackets(UDPQueue::CreateBuffers(10000), UDPQueue::CreateBuffers(10000));
    sourceState = states[sourceState]->SendPackets(connection);
    EXPECT_EQ(sourceState, ConnectionState::WaitingSynAckState);
    auto packets = GetSendPackets(connection);

    EXPECT_EQ(packets.size(), 1);
    EXPECT_EQ(packets.front().GetPacket()->GetPacketType(), PacketType::Syn);

    const ConnectionID destinationID = 1;
    auto [destinationConnection, freePackets] = ListenState::Listen(std::move(packets.front().GetPacket()), destinationID);
    EXPECT_EQ(destinationConnection != nullptr, true);
    destinationConnection->SetInternalFreePackets(UDPQueue::CreateBuffers(10000), UDPQueue::CreateBuffers(10000));
    auto destinationState = ConnectionState::SendingSynAckState;

    destinationState = states[destinationState]->SendPackets(*destinationConnection);
    EXPECT_EQ(destinationState, ConnectionState::DataState);

    {
        auto packets = GetSendPackets(*destinationConnection);
        auto [nextState, freePackets] = states[sourceState]->OnRecvPackets(std::move(packets.front().GetPacket()), connection);
        sourceState = nextState;
        EXPECT_EQ(sourceState, ConnectionState::DataState);
    }
}

TEST(ConnectionStateTest, ConnectionStateDropSynPacket)
{
    auto states = GetStates();

    const ConnectionAddr destinationAddress;
    const ConnectionID sourceID = 1;
    auto sourceState = ConnectionState::SendingSynState;
    Connection connection(sourceState, destinationAddress, sourceID);
    connection.SetInternalFreePackets(UDPQueue::CreateBuffers(10000), UDPQueue::CreateBuffers(10000));
    sourceState = states[sourceState]->SendPackets(connection);
    EXPECT_EQ(sourceState, ConnectionState::WaitingSynAckState);

    auto packets = GetSendPackets(connection);

    EXPECT_EQ(packets.size(), 1);
    EXPECT_EQ(packets.front().GetPacket()->GetPacketType(), PacketType::Syn);

    {
        sourceState = states[sourceState]->OnTimeOut(connection);
        EXPECT_EQ(sourceState, ConnectionState::SendingSynState);
    }

    {
        sourceState = states[sourceState]->SendPackets(connection);
        EXPECT_EQ(sourceState, ConnectionState::WaitingSynAckState);

        packets = GetSendPackets(connection);

        EXPECT_EQ(packets.size(), 1);
        EXPECT_EQ(packets.front().GetPacket()->GetPacketType(), PacketType::Syn);
    }

    const ConnectionID destinationID = 1;
    auto [destinationConnection, freePackets] = ListenState::Listen(std::move(packets.front().GetPacket()), destinationID);
    EXPECT_EQ(destinationConnection != nullptr, true);
    destinationConnection->SetInternalFreePackets(UDPQueue::CreateBuffers(10000), UDPQueue::CreateBuffers(10000));
    auto destinationState = ConnectionState::SendingSynAckState;

    {
        destinationState = states[destinationState]->SendPackets(*destinationConnection);
        EXPECT_EQ(destinationState, ConnectionState::DataState);

        packets = GetSendPackets(*destinationConnection);

        EXPECT_EQ(packets.size(), 1);
        EXPECT_EQ(packets.front().GetPacket()->GetPacketType(), PacketType::SynAck);
    }

    {
        auto [nextState, freePackets] = states[sourceState]->OnRecvPackets(std::move(packets.front().GetPacket()), connection);
        sourceState = nextState;
        EXPECT_EQ(sourceState, ConnectionState::DataState);
    }
}

} // namespace FastTransport::Protocol
