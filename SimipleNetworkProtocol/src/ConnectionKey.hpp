#pragma once

#include "ConnectionAddr.hpp"
#include "HeaderBuffer.hpp"

namespace FastTransport::Protocol {
class ConnectionKey {
public:
    ConnectionKey(ConnectionAddr addr, ConnectionID id)
        : _id(id)
        , _dstAddr(addr)
    {
    }
    ConnectionID _id; // NOLINT(misc-non-private-member-variables-in-classes)
    ConnectionAddr _dstAddr; // NOLINT(misc-non-private-member-variables-in-classes)

    bool operator==(const ConnectionKey that) const // NOLINT(fuchsia-overloaded-operator)
    {
        if (_dstAddr == that._dstAddr) {
            int a = 0;
            a++;
        }

        return _id == that._id && _dstAddr == that._dstAddr;
    }
};
} // namespace FastTransport::Protocol

namespace std {
template <>
struct hash<FastTransport::Protocol::ConnectionKey> { // NOLINT(altera-struct-pack-align)
    std::size_t operator()(const FastTransport::Protocol::ConnectionKey& key) const // NOLINT(fuchsia-overloaded-operator)
    {
        return key._id;
    }
};

} // namespace std
