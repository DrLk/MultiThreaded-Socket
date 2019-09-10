#pragma once

#include "IRecvQueue.h"
#include "ISendQueue.h"


namespace FastTransport
{
    namespace Protocol
    {
        class IConnectionState;
        class Connection
        {
        public:
            Connection(const IRecvQueue&& recvQueue, ISendQueue&& sendQueue) : _recvQueue(std::move(recvQueue)), _sendQueue(std::move(sendQueue))
            {
            }
            void OnRecvPackets(std::shared_ptr<BufferOwner>& packet);
            void ProcessAcks(const SelectiveAckBuffer& acks);
            void ProcessPackets(std::shared_ptr<BufferOwner>& packet);

        private:
            IRecvQueue _recvQueue;
            ISendQueue _sendQueue;
            IConnectionState* _state;
            ConnectionIDType _id;
        };
    }
}
