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
            FastTransportContext()
            {

            }
            IConnection* CreateConnection();
            IConnection* AcceptConnection();

            void Run();

        private:
            std::unordered_map<ConnectionKey, Connection> _connections;
        };



    }
}
