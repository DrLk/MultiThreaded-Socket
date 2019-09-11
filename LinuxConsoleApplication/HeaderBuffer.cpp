#include "HeaderBuffer.h"

#include "BufferOwner.h"

namespace FastTransport
{
    namespace Protocol
    {
        void HeaderBuffer::SetHeader(PacketType packetType, ConnectionID connectionID, SeqNumberType seqNumber)
        {
            const MagicNumber _MagicNumber = 0x12345678;
            _header.SetMagic(_MagicNumber);
            _header.SetPacketType(packetType);
            _header.SetConnectionID(connectionID);
            _header.SetSeqNumber(seqNumber);
        }


        bool HeaderBuffer::IsValid() const
        {
            if (_header.size() < sizeof(Header))
                return false;

            const MagicNumber _MagicNumber = 0x12345678;
            return _header.GetMagic() != _MagicNumber;
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