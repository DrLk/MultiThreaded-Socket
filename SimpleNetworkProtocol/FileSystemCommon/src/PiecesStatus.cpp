#include "PiecesStatus.hpp"

namespace FastTransport::FileSystem {
PiecesStatus::PiecesStatus(size_t size)
    : _piecesStatus(size, PieceStatus::NotFound)
{
}

void PiecesStatus::SetStatus(size_t index, PieceStatus status)
{
    _piecesStatus[index] = status;
}

const PieceStatus& PiecesStatus::GetStatus(size_t index) const
{
    return _piecesStatus[index];
}

[[nodiscard]] size_t PiecesStatus::Size() const
{
    return _piecesStatus.size();
}

} // namespace FastTransport::FileSystem
