#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>

#include "ConnectionAddr.hpp"
#include "HeaderTypes.hpp"

namespace FastTransport::Protocol {
class ConnectionKey {
public:
    ConnectionKey(const ConnectionAddr& addr, ConnectionID connectionID);
    [[nodiscard]] uint16_t GetID() const;
    [[nodiscard]] const ConnectionAddr& GetDestinaionAddr() const;

    bool operator==(const ConnectionKey& that) const; // NOLINT(fuchsia-overloaded-operator)

    struct HashFunction {
        size_t operator()(const ConnectionKey& key) const; // NOLINT(fuchsia-overloaded-operator)
    };

private:
    ConnectionID _id;
    ConnectionAddr _dstAddr;
};

std::ostream& operator<<(std::ostream& stream, const ConnectionKey& key); // NOLINT(fuchsia-overloaded-operator)
} // namespace FastTransport::Protocol