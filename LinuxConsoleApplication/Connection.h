#pragma once

#include <vector>

#include "IRecvQueue.h"
#include "ISendQueue.h"
#include "ConnectionAddr.h"


namespace FastTransport
{
    namespace Protocol
    {
        class IConnectionState;

        class IConnection
        {
            virtual void Send(const std::vector<char>& data) = 0;
            virtual std::vector<char> Recv(int size) = 0;
        };

        class Connection : public IConnection
        {
        public:
            Connection(const ConnectionAddr& addr, ConnectionID id) : _addr(addr), _id(id)
            {
            }

            virtual void Send(const std::vector<char>& data) override;
            virtual std::vector<char> Recv(int size) override;


            void OnRecvPackets(std::shared_ptr<BufferOwner>& packet);
            void ProcessAcks(const SelectiveAckBuffer& acks);
            void ProcessPackets(std::shared_ptr<BufferOwner>& packet);

        private:
            IRecvQueue _recvQueue;
            ISendQueue _sendQueue;
            IConnectionState* _state;
            ConnectionAddr _addr;
            ConnectionID _id;
        };
    }
}
