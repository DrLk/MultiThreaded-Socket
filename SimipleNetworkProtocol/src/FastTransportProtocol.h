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

            IPacket::PairList OnReceive(IPacket::List&& packet);
            std::function<void(OutgoingPacket::List& packets)> OnSend;

            IConnection* Accept();
            IConnection* Connect(const ConnectionAddr&);

            ConnectionID GenerateID()
            {
                static ConnectionID _nextID = 0;
                // TODO: Check after overflow
                return ++_nextID;
            }

        private:
            LockedList<IPacket::Ptr> _freeSendPackets;
            LockedList<IPacket::Ptr> _freeRecvPackets;

            ListenState _listen;
            std::unordered_map<ConnectionKey, Connection*> _connections;
            std::vector<Connection*> _incomingConnections;

            OutgoingPacket::List Send(OutgoingPacket::List& packets);

            UDPQueue _udpQueue;

            std::thread _sendContextThread;
            std::thread _recvContextThread;

            std::atomic<bool> _shutdownContext;

            IPacket::PairList OnReceive(IPacket::Ptr&& packet);

            static void SendThread(FastTransportContext& context);
            static void RecvThread(FastTransportContext& context);

            void ConnectionsRun();
            void SendQueueStep();
            void RecvQueueStep();
            void CheckRecvQueue();

        };
    }
}
