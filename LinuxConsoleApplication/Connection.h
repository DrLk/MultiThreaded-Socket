#pragma once

#include <vector>

#include "IRecvQueue.h"
#include "ISendQueue.h"
#include "ConnectionKey.h"


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
            Connection(const ConnectionAddr& addr, ConnectionID myID, ConnectionID destinationID) : _key(addr, myID), _destinationID(destinationID)
            {
            }

            virtual void Send(const std::vector<char>& data) override
            {
                throw std::runtime_error("Not implemented");
            }

            virtual std::vector<char> Recv(int size) override
            {
                throw std::runtime_error("Not implemented");
            }


            void OnRecvPackets(std::shared_ptr<BufferOwner>& packet);
            void ProcessAcks(const SelectiveAckBuffer& acks);
            void ProcessPackets(std::shared_ptr<BufferOwner>& packet);

            ConnectionKey GetConnectionKey() const;

        private:
            IRecvQueue _recvQueue;
            ISendQueue _sendQueue;
            IConnectionState* _state;
            ConnectionKey _key;
            ConnectionID _destinationID;
        };
    }
}
