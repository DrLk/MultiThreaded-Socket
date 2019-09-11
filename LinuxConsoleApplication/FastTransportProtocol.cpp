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
                Connection* connection = new Connection(dstAddr, GenerateID(), 0);
                _connections.insert({ connection->GetConnectionKey(), connection });

                {
                    std::lock_guard<std::mutex> lock(_freeBuffers._mutex);
                    BufferOwner::ElementType element = _freeBuffers.back();
                    _freeBuffers.pop_back();

                    BufferOwner::Ptr synPacket = std::make_shared<BufferOwner>(_freeBuffers, std::move(element));
                    synPacket->GetHeader().SetPacketType(PacketType::SYN);
                    synPacket->GetHeader().SetConnectionID(1);
                    OnSend(synPacket);
                }


                return connection;

        }


        void FastTransportContext::OnReceive(BufferOwner::Ptr& packet)
        {
            HeaderBuffer header = packet->GetHeaderBuffer();
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
                    _incomingConnections.push_back(connection);
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
    }
}
