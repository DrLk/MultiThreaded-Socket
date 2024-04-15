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

    auto packets = writer.GetWritedPackets();
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
    const int packetsSize = 100;
    for (int i = 0; i < packetsSize; i++) {
        auto packet = std::make_unique<Packet>(1500);
        std::array<std::byte, 1000> payload {};
        packet->SetPayload(payload);
        freePackets.push_back(std::move(packet));
    }

    MessageWriter writer(std::move(freePackets));

    auto writedPackets = writer.GetWritedPackets();
    EXPECT_TRUE(writedPackets.empty());

    auto packets = writer.GetPackets();
    EXPECT_EQ(packets.size(), packetsSize);
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

    EXPECT_EQ(sendPackets.size(), 1);
    writer << std::move(sendPackets);

    auto writedPackets = writer.GetWritedPackets();
    EXPECT_EQ(writedPackets.size(), 2);
    EXPECT_THAT(writedPackets.back()->GetPayload(), ::testing::ElementsAreArray(testData.begin(), testData.end()));
}

TEST(MessageWriter, WriteTrivialAndIPacketList)
{
    IPacket::List packets;
    for (int i = 0; i < 100; i++) {
        auto packet = std::make_unique<Packet>(1500);
        std::array<std::byte, 1000> payload {};
        packet->SetPayload(payload);
        packets.push_back(std::move(packet));
    }

    MessageWriter writer(std::move(packets));

    const int value = 9575;
    writer << value;

    auto dataPackates = writer.GetDataPackets(10);
    std::array<std::byte, 1000> testData {};
    for (auto& byte : testData) {
        byte = std::byte { 64 };
    }
    for (auto& packet : dataPackates) {
        packet->SetPayload(testData);
    }

    EXPECT_EQ(dataPackates.size(), 10);
    for (auto& packet : dataPackates) {
        EXPECT_THAT(packet->GetPayload(), ::testing::ElementsAreArray(testData.begin(), testData.end()));
    }

    writer << std::move(dataPackates);
    writer << value;

    auto writedPackets = writer.GetWritedPackets();
    EXPECT_EQ(writedPackets.size(), 12);

    IPacket::List expectedData;
    auto begin = writedPackets.begin();
    begin++;
    expectedData.splice(writedPackets, begin, --writedPackets.end());
    EXPECT_EQ(expectedData.size(), 10);
    for (auto& packet : expectedData) {
        EXPECT_THAT(packet->GetPayload(), ::testing::ElementsAreArray(testData.begin(), testData.end()));
    }
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

TEST(MessageWriter, WriteBigSpan)
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

    writer << std::span(testData);
}
} // namespace FastTransport::Protocol
