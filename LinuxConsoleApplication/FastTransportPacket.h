#pragma once

#include "vector"


namespace FastTransport
{
    namespace   Protocol
    {
        enum PacketType : short
        {
            NONE = 0,
            SYN = 1,
            ACK = 2,
            SYN_ACK = 3,

        };

        typedef unsigned short ConnectionID;
        typedef unsigned int SeqNumber;
        typedef unsigned int PackNumber;
#pragma pack(push, 1)
        struct FastTransportPacket
        {
            PacketType _packetType;
            ConnectionID _connectionID;
            SeqNumber _seqNumber;
            PackNumber _packNumber;
            std::vector<SeqNumber> _ackNumbers;

            std::vector<char> _data;

        };


#pragma pack(pop)
    }
}
