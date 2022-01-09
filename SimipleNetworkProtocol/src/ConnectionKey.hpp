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

    [[nodiscard]] unsigned short GetID() const
    {
        return _id;
    }

    [[nodiscard]] const ConnectionAddr& GetDestinaionAddr() const
    {
        return _dstAddr;
    }

private:
    ConnectionID _id;
    ConnectionAddr _dstAddr;
};
} // namespace FastTransport::Protocol

namespace std {
template <>
struct hash<FastTransport::Protocol::ConnectionKey> { // NOLINT(altera-struct-pack-align)
    std::size_t operator()(const FastTransport::Protocol::ConnectionKey& key) const // NOLINT(fuchsia-overloaded-operator)
    {
        return key.GetID();
    }
};

template <>
struct equal_to<FastTransport::Protocol::ConnectionKey> { // NOLINT(altera-struct-pack-align)
    bool operator()(const FastTransport::Protocol::ConnectionKey& lhs, const FastTransport::Protocol::ConnectionKey& rhs) const // NOLINT(fuchsia-overloaded-operator)
    {
        using argument_type = FastTransport::Protocol::ConnectionKey;
        using result_type = bool;
        return lhs.GetID() == rhs.GetID() && lhs.GetDestinaionAddr() == rhs.GetDestinaionAddr();
    }
};

} // namespace std
