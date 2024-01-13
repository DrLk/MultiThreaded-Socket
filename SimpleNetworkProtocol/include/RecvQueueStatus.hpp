#pragma once

namespace FastTransport::Protocol {

enum class RecvQueueStatus {
    Invalid,
    New,
    Full,
    Duplicated
};

} // namespace FastTransport::Protocol
