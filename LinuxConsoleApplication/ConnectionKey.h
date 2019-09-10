#pragma once

#include "ConnectionAddr.h"
#include "HeaderBuffer.h"

namespace FastTransport
{
    namespace Protocol
    {
        class ConnectionKey
        {
        public:
            ConnectionKey(ConnectionID id, ConnectionAddr addr) : _id(id), _addr(addr) { }
            ConnectionID _id;
            ConnectionAddr _addr;

            bool operator==(const ConnectionKey that) const
            {
                return _id == that._id && _addr == that._addr;
            }
        };
    }
}

namespace std
{
    template <>
    struct hash<FastTransport::Protocol::ConnectionKey>
    {
        std::size_t operator()(const FastTransport::Protocol::ConnectionKey& key) const
        {
            return key._id;
        }
    };

}
