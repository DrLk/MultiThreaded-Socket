#pragma once

#include <chrono>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include "HeaderBuffer.hpp"
#include "IPacket.hpp"
#include "LockedList.hpp"

namespace FastTransport::Protocol {

using namespace FastTransport::Containers;

class Packet : public IPacket {
public:
    using BufferType = LockedList<ElementType>;
    using Ptr = std::shared_ptr<Packet>;

    Packet(const Packet& that) = delete;
    Packet& operator=(const Packet& that) = delete;
    Packet(Packet&& that) = delete;
    Packet& operator=(Packet&& that) = delete;

    explicit Packet(int size)
        : _element(size)
        , _time(0)
    {
    }

    ~Packet() override = default;

    SelectiveAckBuffer GetAcksBuffer() override
    {
        throw std::runtime_error("Not Implemented");
    }

    SelectiveAckBuffer::Acks GetAcks() override
    {
        return { _element.data(), _element.size() };
    }

    Header GetHeader() override
    {
        return { _element.data(), _element.size() };
    }

    PayloadBuffer::Payload GetPayload() override
    {
        return { _element.data(), _element.size() };
    }

    ConnectionAddr& GetDstAddr() override
    {
        return _dstAddr;
    }

    // TODO: remove and use GetDstAddr for assign
    void SetAddr(const ConnectionAddr& addr) override
    {
        _dstAddr = addr;
    }

    [[nodiscard]] std::chrono::nanoseconds GetTime() const
    {
        return _time;
    }

    void SetTime(const std::chrono::nanoseconds& time)
    {
        _time = time;
    }

    ElementType& GetElement() override
    {
        return _element;
    }

    void Copy(const IPacket& packet) override
    {
        const auto& that = dynamic_cast<const Packet&>(packet);

        _srcAddr = that._srcAddr;
        _dstAddr = that._dstAddr;
        _time = that._time;
        _element = that._element;
    }

private:
    ElementType _element;

    ConnectionAddr _srcAddr;
    ConnectionAddr _dstAddr;
    std::chrono::nanoseconds _time;
};

} // namespace FastTransport::Protocol
