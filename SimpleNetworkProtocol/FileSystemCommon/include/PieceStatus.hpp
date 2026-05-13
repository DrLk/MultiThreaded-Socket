#pragma once

namespace FastTransport::FileSystem {
enum class PieceStatus {
    NotFound,
    InFlight,
    InCache,
    OnDisk,
};

} // namespace FastTransport::FileSystem
