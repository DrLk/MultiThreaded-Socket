#pragma once

#include <memory>

namespace FastTransport
{
    namespace Protocol
    {
        class Connection;
        class BufferOwner;

        class IConnectionState
        {
        public:
            virtual ~IConnectionState() = 0;
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) = 0;
            virtual IConnectionState* SendPackets() = 0;
            virtual IConnectionState* Listen() = 0;
            virtual IConnectionState* Connect() = 0;
            virtual IConnectionState* Close() = 0;
        };

        class BasicSocketState : public IConnectionState
        {
        public:
        private:
        };

        class ConnectedState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket);
        };
    }
}

