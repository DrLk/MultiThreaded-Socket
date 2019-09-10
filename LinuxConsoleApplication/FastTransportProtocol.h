#pragma once

#include <unordered_map>
#include <memory>

#include "Connection.h"
/*#include <list>
#include <unordered_map>
#include <exception>
#include <algorithm>
#include "BufferOwner.h"*/

namespace FastTransport
{
    namespace Protocol
    {
        class FastTransport
        {
        public:
            FastTransport()
            {

            }

            void Run();

        private:
            std::unordered_map<ConnectionIDType, Connection> _connections;
        };



    }
}
