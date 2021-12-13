#pragma once

#include "ISendQueue.h"

#include <vector>
#include <atomic>

#include "LockedList.h"
#include "OutgoingPacket.h"
#include "HeaderBuffer.h"

namespace FastTransport::Protocol
{
    class IPacket;
    class OutgoingPacket;

    class SendQueue : public ISendQueue
    {
        typedef std::vector<char> DataType;

    public:

        SendQueue() : _nextPacketNumber(-1) { }

        void SendPacket(DataType data)
        {

        }

        void SendPacket(IPacket::Ptr&& packet, bool needAck = false);
        void ReSendPackets(OutgoingPacket::List&& packets);

        //make list of list to get fast 1k packets
        OutgoingPacket::List GetPacketsToSend();

    private:
        LockedList<OutgoingPacket> _needToSend;
        std::atomic<SeqNumberType> _nextPacketNumber;

    };
}
