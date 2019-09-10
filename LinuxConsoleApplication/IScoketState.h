#pragma once

#include <memory>

#include "IRecvQueue.h"
#include "ISendQueue.h"
#include "Packet.h"
#include "BufferOwner.h"

namespace FastTransport
{
    namespace Protocol
    {
        class ISocketState;
        class Socket
        {
        public:
            IRecvQueue _recvQueue;
            ISendQueue _sendQueue;
            ISocketState* _state;
        };

        class ISocketState
        {
        public:
            virtual ~ISocketState() = 0;
            virtual ISocketState* OnRecvPackets() = 0;
            virtual ISocketState* SendPackets() = 0;
            virtual Listen() = 0;
            virtual ISocketState* Connect() = 0;
            virtual ISocketState* Close() = 0;
        };

        class BasicSocketState : public ISocketState
        {
        public:
            BasicSocketState(std::unique_ptr<IRecvQueue>&& _recvQueue, std::unique_ptr <ISendQueue>&& _sendQueue)
        private:
        };


        class ConnectedState : public BasicSocketState
        {
        public:
            virtual ISocketState* OnRecvPackets(std::shared_ptr<BufferOwner>& _packet, Socket& socket)
            {
                if (_packet->GetAcks())
                return this;

            }


        }


    }
}
