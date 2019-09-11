#pragma once

#include <unordered_map>
#include <memory>

#include "ConnectionKey.h"
#include "Connection.h"
#include "IConnectionState.h"
#include "BufferOwner.h"

namespace FastTransport
{
    namespace Protocol
    {
        class FastTransportContext
        {
        public:
            FastTransportContext()
            {

            }

            void OnReceive(BufferOwner::Ptr& packet);
            void Send(BufferOwner::Ptr& packet);

            IConnection* Accept();

            void Run();

            ConnectionID GenerateID()
            {
                static ConnectionID _nextID = 0;
                // TODO: Check after overflow
                return ++_nextID;
            }

        private:
            ListenState _listen;
            std::unordered_map<ConnectionKey, Connection*> _connections;
            std::vector<Connection*> _incomingConnections;
        };



    }
}
