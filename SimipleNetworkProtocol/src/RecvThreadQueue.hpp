#pragma once

#include <list>
#include <memory>
#include <vector>

#include "IPacket.hpp"

namespace FastTransport::Protocol {
class RecvThreadQueue {
public:
    RecvThreadQueue() = default;
    ~RecvThreadQueue() = default;

    RecvThreadQueue(const RecvThreadQueue&) = delete;
    RecvThreadQueue& operator=(const RecvThreadQueue&) = delete;
    RecvThreadQueue(RecvThreadQueue&&) = delete;
    RecvThreadQueue& operator=(RecvThreadQueue&&) = delete;

    IPacket::List _recvThreadQueue; // NOLINT(misc-non-private-member-variables-in-classes)
};
} // namespace FastTransport::Protocol
