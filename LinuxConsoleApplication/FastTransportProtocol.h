#pragma once

#include <list>
#include <unordered_map>
#include <exception>
#include <algorithm>
#include "BufferOwner.h"

namespace FastTransport
{
    namespace Protocol
    {
        class Connection
        {
        public:
            Connection(ConnectionIDType id) : _id(id)
            {

            }

            /*void Send(FastTransportPacket& data)
            {

            }

            void OnConnect()
            {

            }

            
            void TryConnect()
            {
                FastTransportPacket* packet = new FastTransportPacket();
                packet->_connectionID = _id;
                packet->_packetType = PacketType::SYN;
                packet->_seqNumber = 0;

                //_inFlightPackets.insert({ packet->_seqNumber, packet });
                Send(*packet);
            }

            void OnRecieved(FastTransportPacket& packet)
            { 
                FastTransportPacket answer;
                switch (packet._packetType)
                {
                    case PacketType::SYN:
                    {
                        answer._packetType = PacketType::SYN_ACK;
                        OnConnect();
                        break;
                    }
                    case PacketType::SYN_ACK:
                    {
                        answer._packetType = PacketType::ACK;
                        OnConnect();
                        for (SeqNumber ack : packet._ackNumbers)
                        {
                            auto it = _inFlightPackets.find(ack);
                            delete it->second;

                            _inFlightPackets.erase(it);
                        }

                        break;
                    }
                    case PacketType::ACK:
                    {
                        //std::copy()
                        break;
                    }
                    default:
                        throw std::runtime_error("unknown packet type");
                }

            }
        private:
            int _mtuSize = 1300;
            SeqNumber _seqNumber = 0;
            //std::list<FastProtocol::Protocol::Packet*> _inFlightPackets;
            std::list<FastTransportPacket*> _sendQueue;
            std::list<SeqNumber> _sendAckQueue;

            std::list<FastTransportPacket*> _recvQueue;
*/
            ConnectionIDType _id;
        };

    }
}
