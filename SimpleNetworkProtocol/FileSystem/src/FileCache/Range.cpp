#include "Range.hpp"

#include "IPacket.hpp"

namespace FastTransport::FileSystem::FileCache {

    Range::Range(off_t offset, size_t size, Data&& data)
        : _offset(offset)
        , _size(size)
        , _data(std::move(data))
    {
    }

    Range::Range(Range&& other) noexcept
        : _offset(other._offset)
        , _size(other._size)
        , _data(std::move(other._data))
    {
    }

    Range& Range::operator=(Range&& other) noexcept
    {
        _offset = other._offset;
        _size = other._size;
        _data = std::move(other._data);
        return *this;
    }

    Range::~Range() = default;

    [[nodiscard]] off_t Range::GetOffset() const
    {
        return _offset;
    }

    [[nodiscard]] size_t Range::GetSize() const
    {
        return _size;
    }

    [[nodiscard]] const Range::Data& Range::GetPackets() const
    {
        return _data;
    }

    [[nodiscard]] Range::Data& Range::GetPackets()
    {
        return _data;
    }

    void Range::AddRef()
    {
        _counter++;
    }

    void Range::Release()
    {
        if (--_counter == 0) {
            delete this;
        }
    }

    bool Range::operator<(const Range& other) const // NOLINT(fuchsia-overloaded-operator)
    {
        return _offset < other._offset;
    }

}  // namespace FastTransport::FileSystem::FileCache
