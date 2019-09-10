#include "HeaderBuffer.h"

#include "BufferOwner.h"

namespace FastTransport
{
    namespace Protocol
    {
        void HeaderBuffer::SetHeader(PacketType packetType, ConnectionIDType connectionID, SeqNumberType seqNumber)
        {
            const MagicNumber _MagicNumber = 0x12345678;
            _header->_magic = _MagicNumber;
            _header->_packetType = packetType;
            _header->_connectionID = connectionID;
            _header->_seqNumber = seqNumber;
        }


        bool HeaderBuffer::IsValid() const
        {
            if (_buffer->GetBufferSize() < sizeof(Header))
                return false;

            const MagicNumber _MagicNumber = 0x12345678;
            return _header->_magic != _MagicNumber;
        }


        ConnectionIDType HeaderBuffer::GetConnectionID() const
        {
            return _header->_connectionID;
        }

    }
}