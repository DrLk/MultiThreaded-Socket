#include "FastTransportProtocol.h"

#include "PeriodicExecutor.h"


namespace FastTransport
{
    namespace Protocol
    {

        static std::list<std::unique_ptr<IPacket>> Recv()
        {
            std::list<std::unique_ptr<IPacket>> result;

            return result;
        }


        FastTransportContext::FastTransportContext(int port) : _shutdownContext(false), 
            _sendContextThread(SendThread, std::ref(*this)), _recvContextThread(RecvThread, std::ref(*this)),
            _udpQueue(port, 2, 1000, 1000)
        {
            _udpQueue.Init();
                /*_freeBuffers.resize(100);
                for (auto buffer : _freeBuffers)
                    buffer.resize(1500);*/
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

        void FastTransportContext::OnReceive(std::list<std::unique_ptr<IPacket>>&& packets)
        {
            for (auto& packet : packets)
                OnReceive(std::move(packet));
        }

        void FastTransportContext::OnReceive(std::unique_ptr<IPacket>&& packet)
        {
            Header header = packet->GetHeader();
            if (!header.IsValid())
            {
                throw std::runtime_error("Not implemented");
            }

            auto connection = _connections.find(ConnectionKey(packet->GetDstAddr(), packet->GetHeader().GetDstConnectionID()));

            if (connection != _connections.end())
            {
                connection->second->OnRecvPackets(std::move(packet));
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
        }

        void FastTransportContext::ConnectionsRun()
        {
            for (auto connection : _connections)
            {
                connection.second->Run();
            }

            SendQueueStep();
        }

        void FastTransportContext::Run()
        {
            while (true)
            {
                std::list<std::unique_ptr<IPacket>> recvBuffers = Recv();

                for (auto& recv : recvBuffers)
                {
                    OnReceive(std::move(recv));
                }


                //std::list<Packet> buffers;
                //Send(buffers);

            }
        }

        void FastTransportContext::SendQueueStep()
        {
            std::list<OutgoingPacket> packets;
            for (auto connection : _connections)
            {
                packets.splice(packets.begin(), connection.second->GetPacketsToSend());
            }

            std::list<OutgoingPacket> inFlightPackets = Send(packets);
            
            std::unordered_map<ConnectionKey, std::list<OutgoingPacket>> connectionOutgoingPackets;
            for (auto& outgoingPacket : inFlightPackets)
            {
                auto& packet = outgoingPacket._packet;
                ConnectionKey key(packet->GetDstAddr(), packet->GetHeader().GetSrcConnectionID());

                connectionOutgoingPackets[key].push_back(std::move(outgoingPacket));

            }

            for (auto& outgoing : connectionOutgoingPackets)
            {
                auto connection = _connections.find(outgoing.first);

                if (connection != _connections.end())
                {
                    connection->second->ProcessSentPackets(std::move(outgoing.second));
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
            std::list<std::unique_ptr<IPacket>> freePackets;

            auto packets = _udpQueue.Recv(std::move(freePackets));

            OnReceive(std::move(packets));
        }

        std::list<OutgoingPacket> FastTransportContext::Send(std::list<OutgoingPacket>& packets)
        {
            if (OnSend || !packets.empty())
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
                }, 20ms);

            while (!context._shutdownContext)
            {
                pe.Run();
            }

        }

    }
}
