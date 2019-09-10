#pragma once

#include <unordered_map>
#include <memory>

#include "ConnectionKey.h"
#include "Connection.h"

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
            std::unordered_map<ConnectionKey, Connection> _connections;
            ConnectionIDType _nextID;
        };



    }
}
