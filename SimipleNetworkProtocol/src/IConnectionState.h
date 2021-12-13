#pragma once

#include <memory>
#include <list>

#include "HeaderBuffer.h"
#include "IPacket.h"

namespace FastTransport
{
    namespace Protocol
    {
        class Connection;
        class Packet;


        enum ConnectionState
        {
            WAIT_SYNACK,
            CONNECTED,
        };

        class IConnectionState
        {
        public:
            virtual ~IConnectionState() { }
            virtual IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) = 0;
            virtual IConnectionState* SendPackets(Connection& connection) = 0;
            virtual IConnectionState* OnTimeOut(Connection& connection) = 0;
        };

        class BasicConnectionState : public IConnectionState
        {
        public:
            virtual IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override 
            {
                IPacket::List freePackets;
                freePackets.push_back(std::move(packet));
                return freePackets;
            }
            virtual IConnectionState* SendPackets(Connection& connection) override { return this; }
            virtual IConnectionState* OnTimeOut(Connection& connection) override { return this; }
        };

        class ListenState
        {
        public:
            Connection* Listen(IPacket::Ptr&& packet, ConnectionID myID);
        };

        class SendingSynState : public BasicConnectionState
        {
        public:
            virtual IConnectionState* SendPackets(Connection& connection) override;
        };

        class WaitingSynState : public BasicConnectionState
        {
        public:
            virtual IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
        };

        class WaitingSynAckState : public BasicConnectionState
        {
        public:
            virtual IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
            virtual IConnectionState* OnTimeOut(Connection& connection) override;
        };

        class SendingSynAckState : public BasicConnectionState
        {
            virtual IConnectionState* SendPackets(Connection& connection) override;
        };

        class DataState : public BasicConnectionState
        {
        public:
            virtual IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
            virtual IConnectionState* SendPackets(Connection& connection) override;
            virtual IConnectionState* OnTimeOut(Connection& connection) override;
        };

        class ClosingState : public BasicConnectionState
        {
        public:
            virtual IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
        };


        class ClosedState : public BasicConnectionState
        {
        public:
            virtual IPacket::List OnRecvPackets(IPacket::Ptr&& packet, Connection& connection) override;
        };
    }
}
