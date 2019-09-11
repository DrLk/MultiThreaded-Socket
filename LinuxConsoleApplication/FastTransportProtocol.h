#pragma once

#include <unordered_map>
#include <memory>

#include "ConnectionKey.h"
#include "Connection.h"
#include "IConnectionState.h"

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
            IConnection* CreateConnection(const ConnectionAddr& addr);
            IConnection* AcceptConnection();

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
        };



    }
}
