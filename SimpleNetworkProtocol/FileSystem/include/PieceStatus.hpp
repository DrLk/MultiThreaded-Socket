#pragma once

namespace FastTransport::FileSystem {
enum class PieceStatus {
    NotFound,
    InCache,
    OnDisk,
};

} // namespace FastTransport::FileSystem
