#include "MessageReader.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <span>

#include "IPacket.hpp"
#include "Packet.hpp"

namespace FastTransport::Protocol {

TEST(MessageReader, Payload)
{
    std::vector<std::uint32_t> testData(100);
    testData[0] = 1;
    testData[1] = 2;
    testData[2] = 3;
    testData[3] = 0xFFFFFFFF;

    IPacket::Ptr packet = std::make_unique<Packet>(1500);
    packet->SetPayload(std::as_bytes(std::span<std::uint32_t>(testData)));
    IPacket::List packets;
    packets.push_back(std::move(packet));
    MessageReader reader(std::move(packets));

    std::array<std::uint32_t, 99> payload {};
    auto bytePayload = std::as_writable_bytes(std::span<std::uint32_t>(payload));
    reader.read(bytePayload.data(), bytePayload.size());
    EXPECT_THAT(payload, ::testing::ElementsAreArray(testData.begin() + 1, testData.end()));
}

} // namespace FastTransport::Protocol
