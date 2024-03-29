#include "Packet.hpp"
#include <gtest/gtest.h>

#include <algorithm>
#include <span>
#include <vector>

#include "HeaderTypes.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {
TEST(PacketTest, Payload)
{
    static constexpr int PacketSize = 1400;
    IPacket::Ptr packet(new Packet(PacketSize));

    std::vector<IPacket::ElementType> writeBytes1(MaxPayloadSize - 1, IPacket::ElementType { 1 });
    packet->SetPayload(writeBytes1);
    auto readBytes1 = packet->GetPayload();
    EXPECT_TRUE(std::equal(readBytes1.begin(), readBytes1.end(), writeBytes1.begin(), writeBytes1.end()));

    std::vector<IPacket::ElementType> writeBytes2(MaxPayloadSize / 2, IPacket::ElementType { 100 });
    packet->SetPayload(writeBytes2);
    auto readBytes2 = packet->GetPayload();
    EXPECT_TRUE(std::equal(readBytes2.begin(), readBytes2.end(), writeBytes2.begin(), writeBytes2.end()));

    std::vector<IPacket::ElementType> writeBytes3(MaxPayloadSize, IPacket::ElementType { 200 });
    packet->SetPayload(writeBytes3);
    auto readBytes3 = packet->GetPayload();
    EXPECT_TRUE(std::equal(readBytes3.begin(), readBytes3.end(), writeBytes3.begin(), writeBytes3.end()));
}

TEST(PacketTest, Acks)
{
    static constexpr int PacketSize = 1400;
    IPacket::Ptr packet(new Packet(PacketSize));

    std::vector<SeqNumberType> writeBytes1(MaxAcksSize - 1, 100);
    packet->SetAcks(writeBytes1);
    auto readBytes1 = packet->GetAcks();
    EXPECT_TRUE(std::equal(readBytes1.begin(), readBytes1.end(), writeBytes1.begin(), writeBytes1.end()));

    std::vector<SeqNumberType> writeBytes2(MaxAcksSize / 2, 200);
    packet->SetAcks(writeBytes2);
    auto readBytes2 = packet->GetAcks();
    EXPECT_TRUE(std::equal(readBytes2.begin(), readBytes2.end(), writeBytes2.begin(), writeBytes2.end()));

    std::vector<SeqNumberType> writeBytes3(MaxAcksSize, 300);
    packet->SetAcks(writeBytes3);
    auto readBytes3 = packet->GetAcks();
    EXPECT_TRUE(std::equal(readBytes3.begin(), readBytes3.end(), writeBytes3.begin(), writeBytes3.end()));
}

TEST(PacketTest, PacketBuffer)
{
    static constexpr int PacketSize = 1400;
    IPacket::Ptr packet(new Packet(PacketSize));

    std::vector<IPacket::ElementType> writeBytes1(MaxPayloadSize - 1, IPacket::ElementType { 1 });
    packet->SetPayload(writeBytes1);
    auto buffer1 = packet->GetBuffer();
    EXPECT_TRUE(std::equal(buffer1.begin() + HeaderSize, buffer1.end(), writeBytes1.begin(), writeBytes1.end()));

    std::vector<IPacket::ElementType> writeBytes2(MaxPayloadSize / 2, IPacket::ElementType { 100 });
    packet->SetPayload(writeBytes2);
    auto buffer2 = packet->GetBuffer();
    EXPECT_TRUE(std::equal(buffer2.begin() + HeaderSize, buffer2.end(), writeBytes2.begin(), writeBytes2.end()));

    std::vector<IPacket::ElementType> writeBytes3(MaxPayloadSize, IPacket::ElementType { 200 });
    packet->SetPayload(writeBytes3);
    auto buffer3 = packet->GetBuffer();
    EXPECT_TRUE(std::equal(buffer3.begin() + HeaderSize, buffer3.end(), writeBytes3.begin(), writeBytes3.end()));

    std::vector<SeqNumberType> writeBytes4(MaxAcksSize, 300);
    packet->SetAcks(writeBytes4);
    auto readBytes3 = packet->GetBuffer();
    EXPECT_TRUE(readBytes3.size() == writeBytes4.size() * sizeof(SeqNumberType) + HeaderSize);
}

} // namespace FastTransport::Protocol
