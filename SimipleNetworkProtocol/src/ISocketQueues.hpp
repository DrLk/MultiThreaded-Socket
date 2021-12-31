#pragma once

#include "IRecvQueue.hpp"
#include "ISendQueue.hpp"

namespace FastTransport::Protocol {
class ISocketQueues {
public:
    ISocketQueues() = default;
    ISocketQueues(const ISocketQueues& that) = default;
    ISocketQueues(ISocketQueues&& that) = default;
    ISocketQueues& operator=(const ISocketQueues& that) = default;
    ISocketQueues& operator=(ISocketQueues&& that) = default;
    virtual ~ISocketQueues() = default;
    virtual IRecvQueue* RecvQueue() = 0;
    virtual ISendQueue* SendQueue() = 0;
};

} // namespace FastTransport::Protocol
