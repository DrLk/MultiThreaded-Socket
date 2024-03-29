#pragma once

#include <list>
#include <utility>
#include <vector>

#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "LockedList.hpp"
#include "RecvQueueStatus.hpp"
#include "SpinLock.hpp"

namespace FastTransport::Protocol {

using FastTransport::Containers::LockedList;

class RecvQueue final : public IRecvQueue {
public:
    RecvQueue();

    [[nodiscard]] std::pair<RecvQueueStatus, IPacket::Ptr> AddPacket(IPacket::Ptr&& packet) override;
    void ProccessUnorderedPackets() override;
    LockedList<IPacket::Ptr>& GetUserData() override;

    [[nodiscard]] std::list<SeqNumberType> GetSelectiveAcks() override;
    [[nodiscard]] SeqNumberType GetLastAck() const override;

private:
    static constexpr int QueueSize = 250000;
    std::vector<IPacket::Ptr> _queue;
    LockedList<IPacket::Ptr> _data;
    Thread::SpinLock _selectiveAcksMutex;
    std::list<SeqNumberType> _selectiveAcks;
    SeqNumberType _beginFullRecievedAck { 0 };
};

} // namespace FastTransport::Protocol
