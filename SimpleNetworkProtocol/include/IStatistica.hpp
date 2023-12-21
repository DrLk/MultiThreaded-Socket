#pragma once

#include <cstdint>

class IStatistica{
    public:
        virtual ~IStatistica() = default;
        virtual uint64_t GetLostPackets() const = 0;
        virtual uint64_t GetSendPackets() const = 0;
        virtual uint64_t GetReceivedPackets() const = 0;
        virtual uint64_t GetDuplicatePackets() const = 0;

};
