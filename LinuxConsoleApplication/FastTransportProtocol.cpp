#include "FastTransportProtocol.h"


namespace FastTransport
{
    namespace Protocol
    {

        static std::list<std::shared_ptr<BufferOwner>> Recv()
        {
            std::list<std::shared_ptr<BufferOwner>> result;
            /*static BufferOwner::BufferType FreeBuffers;

            for (int i = 0; i < 100; i++)
            {
                result.push_back(std::make_shared<BufferOwner>(FreeBuffers));
            }*/

            return result;
        }

        void FastTransportContext::Send(BufferOwner::Ptr& buffer)
        {
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
