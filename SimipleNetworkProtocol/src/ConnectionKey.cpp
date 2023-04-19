#include "ConnectionKey.hpp"

#include <ostream>

namespace FastTransport::Protocol {

ConnectionKey::ConnectionKey(const ConnectionAddr& addr, ConnectionID connectionID)
    : _id(connectionID)
    , _dstAddr(addr)
{
}

[[nodiscard]] uint16_t ConnectionKey::GetID() const
{
    return _id;
}

[[nodiscard]] const ConnectionAddr& ConnectionKey::GetDestinaionAddr() const
{
    return _dstAddr;
}

bool ConnectionKey::operator==(const ConnectionKey& that) const // NOLINT(fuchsia-overloaded-operator)
{
    return _id == that._id && _dstAddr == that._dstAddr;
}

size_t ConnectionKey::HashFunction::operator()(const ConnectionKey& key) const // NOLINT(fuchsia-overloaded-operator)
{
    return key.GetID();
}

std::ostream& operator<<(std::ostream& stream, const ConnectionKey& key) // NOLINT(fuchsia-overloaded-operator)
{
    stream << key.GetDestinaionAddr().GetPort() << ":" << key.GetID();
    return stream;
}
} // namespace FastTransport::Protocol