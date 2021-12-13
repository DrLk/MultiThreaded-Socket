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
            ConnectionKey(ConnectionAddr addr, ConnectionID id) : _id(id), _dstAddr(addr) { }
            ConnectionID _id;
            ConnectionAddr _dstAddr;

            bool operator==(const ConnectionKey that) const
            {
                if (_dstAddr == that._dstAddr)
                {
                    int a = 0;
                    a++;
                }

                return _id == that._id && _dstAddr == that._dstAddr;
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
