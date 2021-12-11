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
            virtual void Send(std::unique_ptr<IPacket>&& data) = 0;
            virtual std::list<std::unique_ptr<IPacket>> Send(std::list<std::unique_ptr<IPacket>>&& data) = 0;
            virtual std::vector<char> Recv(int size) = 0;
            virtual std::list<std::unique_ptr<IPacket>> Recv(std::list<std::unique_ptr<IPacket>>&& freePackets) = 0;
        };

        class Connection : public IConnection
        {
        public:
            Connection(IConnectionState* state, const ConnectionAddr& addr, ConnectionID myID) : _state(state), _key(addr, myID), _destinationID(0), _lastReceivedPacket(DefaultTimeOut)
            {
                for (int i = 0; i < 1000; i++)
                {
                    _freeBuffers.push_back(std::move(std::make_unique<Packet>(1500)));
                }
            }

            virtual void Send(std::unique_ptr<IPacket>&& data) override;
            virtual std::list<std::unique_ptr<IPacket>> Send(std::list<std::unique_ptr<IPacket>>&& data) override;

            virtual std::vector<char> Recv(int size) override;
            virtual std::list<std::unique_ptr<IPacket>> Recv(std::list<std::unique_ptr<IPacket>>&& freePackets) override;


            void OnRecvPackets(std::unique_ptr<IPacket>&& packet);

            const ConnectionKey& GetConnectionKey() const;
            std::list<OutgoingPacket> GetPacketsToSend();

            void ProcessSentPackets(std::list<OutgoingPacket>&& packets);
            void ProcessRecvPackets();

            void Close();

            void Run();

            RecvQueue _recvQueue;
            SendQueue _sendQueue;
            IInflightQueue _inFlightQueue;

        public:
            void SendPacket(std::unique_ptr<IPacket>&& packet, bool needAck = true);

            IConnectionState* _state;
            ConnectionKey _key;
            ConnectionID _destinationID;

            LockedList<std::unique_ptr<IPacket>> _freeBuffers;

            std::list<std::unique_ptr<IPacket>> _sendUserData;
            std::mutex _sendUserDataMutex;
            LockedList<std::vector<char>> _recvUserData;

        private:
            std::chrono::microseconds _lastReceivedPacket;

            static constexpr std::chrono::microseconds DefaultTimeOut = 100ms;

        };
    }
}
