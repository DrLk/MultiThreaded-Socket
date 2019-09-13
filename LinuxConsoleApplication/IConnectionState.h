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
            virtual ~IConnectionState() { }
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) = 0;
            virtual IConnectionState* SendPackets(Connection& socket) = 0;
        };

        class BasicSocketState : public IConnectionState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override { return this; }
            virtual IConnectionState* SendPackets(Connection& socket) override { return this; }
        };

        class ListenState
        {
        public:
            Connection* Listen(std::shared_ptr<BufferOwner>& packet, ConnectionID myID);
        };

        class SynState : public BasicSocketState
        {
        public:
            virtual IConnectionState* SendPackets(Connection& socket) override;
        };

        class WaitingSynState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override;
        };

        class WaitingSynAckState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override;
        };

        class SendingSynAckState : public BasicSocketState
        {
            virtual IConnectionState* SendPackets(Connection& socket) override;
        };

        class DataState : public BasicSocketState
        {
        public:
            virtual IConnectionState* OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket) override;
            virtual IConnectionState* SendPackets(Connection& socket) override;
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

