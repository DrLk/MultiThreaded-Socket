#pragma once

#include <memory>

#include "HeaderBuffer.h"

namespace FastTransport
{
    namespace Protocol
    {
        class Connection;
        class BufferOwner;


        enum ConnectionState
        {
            WAIT_SYNACK,
            CONNECTED,
        };

        class IConnectionState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) = 0;
        };

        class BasicSocketState : public IConnectionState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override { throw std::runtime_error("Not implemented"); }
        };

        class ListenState
        {
        public:
            Connection* Listen(std::shared_ptr<BufferOwner>& packet, ConnectionID myID);
        };

        class WaitingSynAck : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override;
        };

        class ConnectedState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override;
        };

        class ClosingState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override;
        };


        class ClosedState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override;
        };
    }
}

