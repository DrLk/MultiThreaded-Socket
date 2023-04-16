#include "ConnectionKey.hpp"

#include <ostream>

namespace FastTransport::Protocol {
std::ostream& operator<<(std::ostream& stream, const ConnectionKey& key) // NOLINT(fuchsia-overloaded-operator)
{
    stream << key.GetDestinaionAddr().GetPort() << ":" << key.GetID();
    return stream;
}
} // namespace FastTransport::Protocol