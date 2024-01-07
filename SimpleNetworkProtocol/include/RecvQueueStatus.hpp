#pragma once

namespace FastTransport::Protocol {

enum class RecvQueueStatus {
    INVALID,
    NEW,
    FULL,
    DUPLICATED
};

} // namespace FastTransport::Protocol
