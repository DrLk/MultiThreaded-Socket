#include "RecvQueue.hpp"
#include <gtest/gtest.h>

#include <stop_token>
#include <utility>
#include <vector>

#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "LockedList.hpp"
#include "Packet.hpp"

namespace FastTransport::Protocol {
TEST(RecvQueueTest, RecvQueue)
{
    std::unique_ptr<IRecvQueue> queue = std::make_unique<RecvQueue>();

    std::vector<IPacket::Ptr> packets;
    constexpr SeqNumberType beginSeqNumber = 0;
    constexpr int PacketCount = 97;
    for (int i = 0; i < PacketCount; i++) {
        IPacket::Ptr packet(new Packet(1400));
        packet->SetSeqNumber(i + beginSeqNumber);
        packets.push_back(std::move(packet));
    }

    IPacket::List freePackets;
    for (auto& packet : packets) {
        auto [status, freePacket] = queue->AddPacket(std::move(packet));
        if (freePacket) {
            freePackets.push_back(std::move(freePacket));
        }
    }

    queue->ProccessUnorderedPackets();

    auto& userData = queue->GetUserData();
    IPacket::List result;
    if (userData.Wait(std::stop_token())) {
        userData.LockedSwap(result);
    }

    EXPECT_EQ(result.size(), PacketCount);
}
} // namespace FastTransport::Protocol
