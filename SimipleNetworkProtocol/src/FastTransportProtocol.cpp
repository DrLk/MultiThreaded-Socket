#include "FastTransportProtocol.h"


namespace FastTransport
{
    namespace Protocol
    {

        static std::list<std::shared_ptr<BufferOwner>> Recv()
        {
            std::list<std::shared_ptr<BufferOwner>> result;

            return result;
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

        void FastTransportContext::OnReceive(std::list<BufferOwner::Ptr>&& packets)
        {
            for (auto packet : packets)
                OnReceive(packet);
        }

        void FastTransportContext::OnReceive(BufferOwner::Ptr& packet)
        {
            HeaderBuffer::Header header = packet->GetHeader();
            if (!header.IsValid())
            {
                throw std::runtime_error("Not implemented");
            }

            auto connection = _connections.find(ConnectionKey(packet->GetAddr(), packet->GetHeader().GetConnectionID()));

            if (connection != _connections.end())
            {
                connection->second->OnRecvPackets(packet);
            }
            else
            {
                Connection* connection = _listen.Listen(packet, GenerateID());
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
        }

        void FastTransportContext::Run()
        {
            while (true)
            {
                std::list<std::shared_ptr<BufferOwner>> recvBuffers = Recv();

                for (auto& recv : recvBuffers)
                {
                    OnReceive(recv);
                }


                //std::list<BufferOwner> buffers;
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

            if (!packets.empty())
                Send(packets);
        }

        void FastTransportContext::Send(std::list<OutgoingPacket>& packets)
        {
            OnSend(packets);
        }
    }
}
