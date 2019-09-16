#pragma once

#include <vector>
#include <list>

#include "IRecvQueue.h"
#include "ISendQueue.h"
#include "ConnectionKey.h"
#include "LockedList.h"


namespace FastTransport
{
    namespace Protocol
    {
        class IConnectionState;

        class IConnection
        {
        public:
            virtual ~IConnection() { }
            virtual void Send(const std::vector<char>& data) = 0;
            virtual std::vector<char> Recv(int size) = 0;
        };

        class Connection : public IConnection
        {
        public:
            Connection(IConnectionState* state, const ConnectionAddr& addr, ConnectionID myID) : _state(state), _key(addr, myID), _destinationID(0)
            {
                for (int i = 0; i < 1000; i++)
                {
                    _freeBuffers.push_back(BufferOwner::ElementType(1500));
                }
            }

            virtual void Send(const std::vector<char>& data) override;

            virtual std::vector<char> Recv(int size) override;


            void OnRecvPackets(std::shared_ptr<BufferOwner>& packet);

            const ConnectionKey& GetConnectionKey() const;

            std::list<BufferOwner::Ptr>&& GetPacketsToSend();

            void Close();

            void Run();

            IRecvQueue _recvQueue;
            ISendQueue _sendQueue;

        public:
            void SendPacket(std::shared_ptr<BufferOwner>& packet);

            IConnectionState* _state;
            ConnectionKey _key;
            ConnectionID _destinationID;

            LockedList<BufferOwner::ElementType> _freeBuffers;
        };
    }
}
