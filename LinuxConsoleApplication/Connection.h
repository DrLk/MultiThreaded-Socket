#pragma once

#include <vector>

#include "IRecvQueue.h"
#include "ISendQueue.h"


namespace FastTransport
{
    namespace Protocol
    {
        class IConnectionState;

        class IConnection
        {
            virtual void Send(const std::vector<char>& data) = 0;
            virtual std::vector<char> Recv(int size) = 0;
            virtual IConnection* Listen() = 0;
            virtual IConnection* Connect() = 0;
        };

        class Connection : public IConnection
        {
        public:
            Connection()
            {
            }

            virtual void Send(const std::vector<char>& data) override;
            virtual std::vector<char> Recv(int size) override;
            virtual IConnection* Listen() override;
            virtual IConnection* Connect() override;


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
