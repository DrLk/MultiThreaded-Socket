#pragma once

#include <unordered_map>
#include <memory>

#include "ConnectionKey.h"
#include "Connection.h"
#include "IConnectionState.h"
#include "BufferOwner.h"
#include <functional>

namespace FastTransport
{
    namespace Protocol
    {
        class FastTransportContext
        {
        public:
            FastTransportContext()
            {
                _freeBuffers.resize(100);
                for (auto buffer : _freeBuffers)
                    buffer.resize(1500);
            }

            void OnReceive(BufferOwner::Ptr& packet);
            std::function<void(BufferOwner::Ptr&)> OnSend;

            IConnection* Accept();
            IConnection* Connect(const ConnectionAddr&);

            void Run();

            void SendQueueStep();
            void RecvQueueStep();

            ConnectionID GenerateID()
            {
                static ConnectionID _nextID = 0;
                // TODO: Check after overflow
                return ++_nextID;
            }

        private:
            BufferOwner::BufferType _freeBuffers;
            ListenState _listen;
            std::unordered_map<ConnectionKey, Connection*> _connections;
            std::vector<Connection*> _incomingConnections;

            void Send(BufferOwner::Ptr& packet) const;
            LockedList<BufferOwner::Ptr> _packets;

        };



    }
}
