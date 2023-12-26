#pragma once

namespace FastTransport::Protocol {

enum class ConnectionState {
    SendingSynState,
    WaitingSynState,
    WaitingSynAckState,
    SendingSynAckState,
    DataState,
    ClosingState,
    ClosedState
};

} // namespace FastTransport::Protocol
