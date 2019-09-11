#pragma once

#include <vector>
#include <list>

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
        public:
            virtual void Send(const std::vector<char>& data) = 0;
            virtual std::vector<char> Recv(int size) = 0;
        };

        class Connection : public IConnection
        {
        public:
            Connection(const ConnectionAddr& addr, ConnectionID myID, ConnectionID destinationID) : _key(addr, myID), _destinationID(destinationID), _sendQueue(new SendQueue()), _recvQueue(new RecvQueue())
            {
            }

            virtual void Send(const std::vector<char>& data) override;

            virtual std::vector<char> Recv(int size) override;


            void OnRecvPackets(std::shared_ptr<BufferOwner>& packet);
            void ProcessAcks(const SelectiveAckBuffer& acks);
            void ProcessPackets(std::shared_ptr<BufferOwner>& packet);

            const ConnectionKey& GetConnectionKey() const;
            SeqNumberType GetCurrentSeqNumber();

            std::list<BufferOwner::Ptr>&& GetPacketsToSend();

        private:
            IConnectionState* _state;
            ConnectionKey _key;
            ConnectionID _destinationID;
            SeqNumberType _seqNumber;
            std::list<BufferOwner::Ptr> _packetsToSend;

            ISendQueue* _sendQueue;
            IRecvQueue* _recvQueue;
        };
    }
}
