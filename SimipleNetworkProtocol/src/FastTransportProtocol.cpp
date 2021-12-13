#include "FastTransportProtocol.h"

#include "PeriodicExecutor.h"


namespace FastTransport
{
    namespace Protocol
    {

        static std::list<IPacket::Ptr> Recv()
        {
            std::list<IPacket::Ptr> result;

            return result;
        }


        FastTransportContext::FastTransportContext(int port) : _shutdownContext(false), 
            _sendContextThread(SendThread, std::ref(*this)), _recvContextThread(RecvThread, std::ref(*this)),
            _udpQueue(port, 2, 1000, 1000)
        {
            for (int i = 0; i < 1000; i++)
            { 
                _freeListenPackets.push_back(std::move(std::make_unique<Packet>(1500)));
            }
            _udpQueue.Init();
        }

        FastTransportContext::~FastTransportContext()
        {
            _shutdownContext = true;

            _sendContextThread.join();
            _recvContextThread.join();
        }

        IConnection* FastTransportContext::Accept()
        {
            if (_incomingConnections.empty())
                return nullptr;

            Connection* connection = _incomingConnections.back();
            _incomingConnections.pop_back();

            _connections.insert({ connection->GetConnectionKey(), connection });

            return connection;
        }

        IConnection* FastTransportContext::Connect(const ConnectionAddr& dstAddr)
        {
            Connection* connection = new Connection(new SendingSynState(), dstAddr, GenerateID());
            _connections.insert({ connection->GetConnectionKey(), connection });

            return connection;

        }

        std::list<IPacket::Ptr> FastTransportContext::OnReceive(std::list<IPacket::Ptr>&& packets)
        {
            std::list<IPacket::Ptr> freePackets;

            for (auto& packet : packets)
            {
                auto packets = OnReceive(std::move(packet));
                freePackets.splice(freePackets.end(), packets);
            }

            return freePackets;
        }

        std::list<IPacket::Ptr> FastTransportContext::OnReceive(IPacket::Ptr&& packet)
        {
            std::list<IPacket::Ptr> freePackets;

            Header header = packet->GetHeader();
            if (!header.IsValid())
            {
                throw std::runtime_error("Not implemented");
            }

            auto connection = _connections.find(ConnectionKey(packet->GetDstAddr(), packet->GetHeader().GetDstConnectionID()));

            if (connection != _connections.end())
            {
                auto packets =connection->second->OnRecvPackets(std::move(packet));
                freePackets.splice(freePackets.end(), packets);
            }
            else
            {
                Connection* connection = _listen.Listen(std::move(packet), GenerateID());
                if (connection)
                {
                    _incomingConnections.push_back(connection);
                    _connections.insert({ connection->GetConnectionKey(), connection });
                }
            }

            return freePackets;
        }

        void FastTransportContext::ConnectionsRun()
        {
            for (auto& [key, connection] : _connections)
            {
                connection->Run();
            }

            SendQueueStep();
        }

        void FastTransportContext::SendQueueStep()
        {
            std::list<OutgoingPacket> packets;
            for (auto& [key, connection] : _connections)
            {
                packets.splice(packets.begin(), connection->GetPacketsToSend());
            }

            std::list<OutgoingPacket> inFlightPackets = Send(packets);
            
            std::unordered_map<ConnectionKey, std::list<OutgoingPacket>> connectionOutgoingPackets;
            for (auto& outgoingPacket : inFlightPackets)
            {
                auto& packet = outgoingPacket._packet;
                ConnectionKey key(packet->GetDstAddr(), packet->GetHeader().GetSrcConnectionID());

                connectionOutgoingPackets[key].push_back(std::move(outgoingPacket));

            }

            for (auto& [connectionKey, outgoingPacket] : connectionOutgoingPackets)
            {
                auto connection = _connections.find(connectionKey);

                if (connection != _connections.end())
                {
                    connection->second->ProcessSentPackets(std::move(outgoingPacket));
                }
                else
                {
                    //needs return free packets to pool
                    throw std::runtime_error("Not implemented");
                }


            }
        }

        void FastTransportContext::RecvQueueStep()
        {
            //TODO: get 1k freePackets
            std::list<IPacket::Ptr> freePackets;
            {
                std::lock_guard lock(_freeListenPackets._mutex);
                freePackets.splice(freePackets.end(), _freeListenPackets);
            }

            auto receivedPackets = _udpQueue.Recv(std::move(freePackets));

            auto packets = OnReceive(std::move(receivedPackets));

            {
                std::lock_guard lock(_freeListenPackets._mutex);
                _freeListenPackets.splice(_freeListenPackets.end(), packets);
            }
        }

        void FastTransportContext::CheckRecvQueue()
        {
            for (auto& [key, connection] : _connections)
            {
                connection->ProcessRecvPackets();
            }
        }

        std::list<OutgoingPacket> FastTransportContext::Send(std::list<OutgoingPacket>& packets)
        {
            if (OnSend)
                OnSend(packets);

            return _udpQueue.Send(std::move(packets));
        }

        void FastTransportContext::SendThread(FastTransportContext& context)
        {
            PeriodicExecutor pe([&context]()
                {
                    context.ConnectionsRun();
                }, 20ms);

            while (!context._shutdownContext)
            {
                pe.Run();
            }

        }

        void FastTransportContext::RecvThread(FastTransportContext& context)
        {
            PeriodicExecutor pe([&context]()
                {
                    context.RecvQueueStep();
                    context.CheckRecvQueue();
                }, 20ms);

            while (!context._shutdownContext)
            {
                pe.Run();
            }

        }

    }
}
