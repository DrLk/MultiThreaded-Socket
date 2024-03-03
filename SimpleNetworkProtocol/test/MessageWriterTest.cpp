#include "MessageWriter.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <vector>

#include "IPacket.hpp"
#include "Packet.hpp"

namespace FastTransport::Protocol {

TEST(MessageWriter, Payload)
{
    IPacket::List freePackets;
    for (int i = 0; i < 1; i++) {
        auto packet = std::make_unique<Packet>(1500);
        std::array<std::byte, 1000> payload {};
        packet->SetPayload(payload);
        freePackets.push_back(std::move(packet));
    }

    MessageWriter writer(std::move(freePackets));

    int value = 957;
    writer << value;
    writer << value;
    writer << value;

    value = 958;
    writer << value;
    writer << value;
    writer << value;

    auto packets = writer.GetPackets();
    EXPECT_EQ(packets.size(), 1);

    auto& packet = packets.front();
    const auto& payload = packet->GetPayload();
    int packetsSize = 0;
    std::memcpy(&packetsSize, payload.data(), sizeof(packetsSize));
    EXPECT_EQ(packetsSize, 1);

    std::array<int, 3> values {};
    std::memcpy(values.data(), payload.data() + 4, sizeof(values));
    for (auto& value : values) {
        EXPECT_EQ(value, 957);
    }

    std::memcpy(values.data(), payload.data() + 4 + 12, sizeof(values));
    for (auto& value : values) {
        EXPECT_EQ(value, 958);
    }
}

TEST(MessageWriter, Empty)
{
    IPacket::List freePackets;
    int packetsSize = 100;
    for (int i = 0; i < packetsSize; i++) {
        auto packet = std::make_unique<Packet>(1500);
        std::array<std::byte, 1000> payload {};
        packet->SetPayload(payload);
        freePackets.push_back(std::move(packet));
    }

    MessageWriter writer(std::move(freePackets));

    auto packets = writer.GetPackets();
    EXPECT_EQ(packets.size(), packetsSize);

    auto& packet = packets.front();
    const auto& payload = packet->GetPayload();
    int size = 0;
    std::memcpy(&size, payload.data(), sizeof(size));
    EXPECT_EQ(size, packetsSize);
}

TEST(MessageWriter, WriteIPacketList)
{
    IPacket::List freePackets;
    for (int i = 0; i < 100; i++) {
        auto packet = std::make_unique<Packet>(1500);
        std::array<std::byte, 1000> payload {};
        packet->SetPayload(payload);
        freePackets.push_back(std::move(packet));
    }

    MessageWriter writer(std::move(freePackets));

    std::array<std::byte, 1000> testData {};
    for (auto& byte : testData) {
        byte = std::byte { 64 };
    }
    IPacket::List sendPackets;
    sendPackets.push_back(std::make_unique<Packet>(1200));
    sendPackets.back()->SetPayload(testData);

    EXPECT_TRUE(std::equal(testData.begin(), testData.end(), sendPackets.back()->GetPayload().begin(), sendPackets.back()->GetPayload().end()));

    writer << std::move(sendPackets);
    auto packets = writer.GetPackets();
}

TEST(MessageWriter, WriteBigArray)
{
    IPacket::List freePackets;
    for (int i = 0; i < 100; i++) {
        auto packet = std::make_unique<Packet>(1500);
        std::array<std::byte, 1000> payload {};
        packet->SetPayload(payload);
        freePackets.push_back(std::move(packet));
    }

    MessageWriter writer(std::move(freePackets));

    std::vector<std::byte> testData(2000, std::byte { 31 });

    writer.write(testData.data(), testData.size());
}
} // namespace FastTransport::Protocol
