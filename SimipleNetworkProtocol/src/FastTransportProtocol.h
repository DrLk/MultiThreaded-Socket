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

            void OnReceive(std::list<std::unique_ptr<IPacket>>&& packet);
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
            LockedList<std::unique_ptr<IPacket>> _freeBuffers;
            ListenState _listen;
            std::unordered_map<ConnectionKey, Connection*> _connections;
            std::vector<Connection*> _incomingConnections;

            std::list<OutgoingPacket> Send(std::list<OutgoingPacket>& packets);

            std::thread _sendContextThread;
            std::thread _recvContextThread;

            std::atomic<bool> _shutdownContext;

            UDPQueue _udpQueue;

            void OnReceive(std::unique_ptr<IPacket>&& packet);

            static void SendThread(FastTransportContext& context);
            static void RecvThread(FastTransportContext& context);

            void ConnectionsRun();
            void SendQueueStep();
            void RecvQueueStep();
            void CheckRecvQueue();

        };
    }
}
