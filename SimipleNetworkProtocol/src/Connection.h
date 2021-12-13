#pragma once

#include <vector>
#include <list>
#include <chrono>
#include <memory>

#include "IPacket.h"
#include "IRecvQueue.h"
#include "SendQueue.h"
#include "IInFilghtQueue.h"
#include "ConnectionKey.h"
#include "LockedList.h"
#include "Packet.h"

using namespace std::chrono_literals;

namespace FastTransport
{
    namespace Protocol
    {
        class IConnectionState;

        class IConnection
        {
        public:
            virtual ~IConnection() { }
            virtual void Send(IPacket::Ptr&& data) = 0;
            virtual IPacket::List Send(IPacket::List&& data) = 0;
            virtual IPacket::List Recv(IPacket::List&& freePackets) = 0;
        };

        class Connection : public IConnection
        {
        public:
            Connection(IConnectionState* state, const ConnectionAddr& addr, ConnectionID myID) : _state(state), _key(addr, myID), _destinationID(0), _lastReceivedPacket(DefaultTimeOut)
            {
                for (int i = 0; i < 1000; i++)
                {
                    _freeBuffers.push_back(std::move(std::make_unique<Packet>(1500)));
                    _freeRecvPackets.push_back(std::move(std::make_unique<Packet>(1500)));
                }
            }

            virtual void Send(IPacket::Ptr&& data) override;
            virtual IPacket::List Send(IPacket::List&& data) override;

            virtual IPacket::List Recv(IPacket::List&& freePackets) override;


            IPacket::List OnRecvPackets(IPacket::Ptr&& packet);

            const ConnectionKey& GetConnectionKey() const;
            OutgoingPacket::List GetPacketsToSend();

            void ProcessSentPackets(OutgoingPacket::List&& packets);
            void ProcessRecvPackets();

            void Close();

            void Run();

            RecvQueue _recvQueue;
            SendQueue _sendQueue;
            IInflightQueue _inFlightQueue;

        public:
            void SendPacket(IPacket::Ptr&& packet, bool needAck = true);

            IConnectionState* _state;
            ConnectionKey _key;
            ConnectionID _destinationID;

            LockedList<IPacket::Ptr> _freeBuffers;
            LockedList<IPacket::Ptr> _freeRecvPackets;

            IPacket::List _sendUserData;
            std::mutex _sendUserDataMutex;
            LockedList<std::vector<char>> _recvUserData;

        private:
            std::chrono::microseconds _lastReceivedPacket;

            static constexpr std::chrono::microseconds DefaultTimeOut = 100ms;

        };
    }
}
