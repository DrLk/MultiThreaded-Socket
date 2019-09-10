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
            virtual IConnectionState* Connect() = 0;
            virtual IConnectionState* Close() = 0;
        };

        class BasicSocketState : public IConnectionState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override { throw std::runtime_error("Not implemented"); }
        };

        class ListenState
        {
        public:
            Connection* Listen(std::shared_ptr<BufferOwner>& packet);
        };

        class ConnectingState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket);
        };

        class ConnectedState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket);
        };

        class ClosingState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket);
        };


        class ClosedState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override;
        };
    }
}

