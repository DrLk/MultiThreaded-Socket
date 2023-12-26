#pragma once

namespace FastTransport::Protocol {

enum class RecvQueueStatus {
    INVALID,
    NEW,
    FULL,
    DUPLICATE
};

} // namespace FastTransport::Protocol
