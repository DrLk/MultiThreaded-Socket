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
            FastTransportContext() : _nextID(0)
            {

            }
            IConnection* CreateConnection(const ConnectionAddr& addr);
            IConnection* AcceptConnection();

            void Run();

        private:
            ListenState _listen;
            std::unordered_map<ConnectionKey, Connection*> _connections;
            ConnectionID _nextID;
        };



    }
}
