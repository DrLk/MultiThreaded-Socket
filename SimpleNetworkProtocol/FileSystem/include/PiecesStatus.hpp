#pragma once

#include <vector>

#include "PieceStatus.hpp"

namespace FastTransport::FileSystem {

class PiecesStatus {
public:
    explicit PiecesStatus(size_t size);

    void SetStatus(size_t index, PieceStatus status);
    [[nodiscard]] const PieceStatus& GetStatus(size_t index) const;
    [[nodiscard]] size_t Size() const;

private:
    std::vector<PieceStatus> _piecesStatus;
};
} // namespace FastTransport::FileSystem
