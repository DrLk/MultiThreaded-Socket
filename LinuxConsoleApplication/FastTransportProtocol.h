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

            void OnReceive(std::list<BufferOwner::Ptr>&& packet);
            void OnReceive(BufferOwner::Ptr& packet);
            std::function<void(std::list<BufferOwner::Ptr>&& packets)> OnSend;

            IConnection* Accept();
            IConnection* Connect(const ConnectionAddr&);

            void ConnectionsRun();
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

            void Send(std::list<BufferOwner::Ptr>&& packets);

        };
    }
}
