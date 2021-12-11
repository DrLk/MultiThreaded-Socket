#pragma once

#include "ISendQueue.h"

#include <vector>

#include "LockedList.h"
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

        void SendPacket(std::unique_ptr<IPacket>&& packet, bool needAck = false);
        void ReSendPackets(std::list<OutgoingPacket>&& packets);

        //make list of list to get fast 1k packets
        std::list<OutgoingPacket> GetPacketsToSend();

    private:
        LockedList<OutgoingPacket> _needToSend;
        std::atomic<SeqNumberType> _nextPacketNumber;

    };
}