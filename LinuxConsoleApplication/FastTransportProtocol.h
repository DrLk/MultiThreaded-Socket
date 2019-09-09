#pragma once

#include <list>
#include <unordered_map>
#include "FastTransportPacket.h"

namespace FastTransport::Protocol
{
    class FastTransportPacket;
    class FastTransport
    {
    public:

        void Send(std::vector<char>&& data)
        {

        }

        void OnRecieved()
        {

        }
    private:
        //std::list<FastProtocol::Protocol::Packet*> _inFlightPackets;
        std::unordered_map<SeqNumber,FastTransportPacket*> _inFlightPackets2;
        int _mtuSize = 1300;
        
    };

}
