#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "HeaderBuffer.hpp"
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "LockedList.hpp"

namespace FastTransport::Protocol {

using FastTransport::Containers::LockedList;

class RecvQueue final : public IRecvQueue {
public:
    RecvQueue();

    [[nodiscard]] IPacket::Ptr AddPacket(IPacket::Ptr&& packet) override;
    void ProccessUnorderedPackets() override;
    [[nodiscard]] IPacket::List GetUserData() override;

    [[nodiscard]] std::list<SeqNumberType> GetSelectiveAcks() override;
    [[nodiscard]] SeqNumberType GetLastAck() const override;

private:
    static constexpr int QueueSize = 1000;
    std::vector<IPacket::Ptr> _queue;
    LockedList<IPacket::Ptr> _data;
    std::mutex _selectiveAcksMutex;
    std::list<SeqNumberType> _selectiveAcks;
    SeqNumberType _beginFullRecievedAck { 0 };
};

} // namespace FastTransport::Protocol