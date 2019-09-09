#pragma once

#include "vector"


namespace FastTransport::Protocol
{
    typedef unsigned short PacketType;
    typedef unsigned short ConnectionID;
    typedef unsigned int SeqNumber;
    typedef unsigned int PackNumber;
#pragma pack(push, 1)
    struct Packet
    {
        PacketType _packetType;
        ConnectionID _connectionId;
        SeqNumber _seqNumber;
        PackNumber _packNumber;

        std::vector<char> _data;

    };
#pragma pack(pop)
}
