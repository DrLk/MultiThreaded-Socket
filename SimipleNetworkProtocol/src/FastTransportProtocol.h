#pragma once

#include <unordered_map>
#include <functional>
#include <memory>

#include "ConnectionKey.h"
#include "Connection.h"
#include "IConnectionState.h"
#include "IPacket.h"
#include "SpeedController.h"
#include "UDPQueue.h"

namespace FastTransport
{
    namespace Protocol
    {
        class FastTransportContext
        {
        public:
            FastTransportContext(int port);
            ~FastTransportContext();

            IPacket::List OnReceive(IPacket::List&& packet);
            std::function<void(std::list<OutgoingPacket>& packets)> OnSend;

            IConnection* Accept();
            IConnection* Connect(const ConnectionAddr&);

            ConnectionID GenerateID()
            {
                static ConnectionID _nextID = 0;
                // TODO: Check after overflow
                return ++_nextID;
            }

        private:
            LockedList<IPacket::Ptr> _freeBuffers;
            LockedList<IPacket::Ptr> _freeListenPackets;

            ListenState _listen;
            std::unordered_map<ConnectionKey, Connection*> _connections;
            std::vector<Connection*> _incomingConnections;

            std::list<OutgoingPacket> Send(std::list<OutgoingPacket>& packets);

            std::thread _sendContextThread;
            std::thread _recvContextThread;

            std::atomic<bool> _shutdownContext;

            UDPQueue _udpQueue;

            IPacket::List OnReceive(IPacket::Ptr&& packet);

            static void SendThread(FastTransportContext& context);
            static void RecvThread(FastTransportContext& context);

            void ConnectionsRun();
            void SendQueueStep();
            void RecvQueueStep();
            void CheckRecvQueue();

        };
    }
}
