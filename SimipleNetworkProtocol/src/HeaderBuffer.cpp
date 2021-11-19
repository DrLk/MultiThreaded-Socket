#include "HeaderBuffer.h"

namespace FastTransport
{
    namespace Protocol
    {
        void HeaderBuffer::SetHeader(PacketType packetType, ConnectionID connectionID, SeqNumberType seqNumber)
        {
            _header.SetMagic();
            _header.SetPacketType(packetType);
            _header.SetConnectionID(connectionID);
            _header.SetSeqNumber(seqNumber);
        }


        bool HeaderBuffer::IsValid() const
        {
            return _header.IsValid();
        }

        ConnectionID HeaderBuffer::GetConnectionID() const
        {
            return _header.GetConnectionID();
        }

        PacketType HeaderBuffer::GetType() const
        {
            return _header.GetPacketType();
        }

        SeqNumberType HeaderBuffer::GetSeqNumber() const
        {
            return _header.GetSeqNumber();
        }
    }
}