#include "SendQueue.h"

#include "IPacket.h"
#include "OutgoingPacket.h"

#include <limits>


namespace FastTransport::Protocol
{
    void SendQueue::SendPacket(IPacket::Ptr&& packet, bool needAck/* = false*/)
    {
        Header header = packet->GetHeader();
        header.SetMagic();

        if (needAck)
            header.SetSeqNumber(++_nextPacketNumber);
        else
            header.SetSeqNumber((std::numeric_limits<SeqNumberType>::max)());

        {
            std::lock_guard lock(_needToSend._mutex);
            _needToSend.emplace_back(std::move(packet), needAck);
        }
    }

    void SendQueue::ReSendPackets(OutgoingPacket::List&& packets)
    {
        std::lock_guard lock(_needToSend._mutex);
        _needToSend.splice(_needToSend.end(), std::move(packets));
    }

    //make list of list to get fast 1k packets
    OutgoingPacket::List SendQueue::GetPacketsToSend()
    {
        OutgoingPacket::List result;
        {
            std::lock_guard lock(_needToSend._mutex);
            result = std::move(_needToSend);
        }

        return result;
    }
}