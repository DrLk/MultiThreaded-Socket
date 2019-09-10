#include "FastTransportProtocol.h"


namespace FastTransport
{
    namespace Protocol
    {

        static std::list<std::shared_ptr<BufferOwner>> Recv()
        {
            static BufferOwner::BufferType FreeBuffers;

            std::list<std::shared_ptr<BufferOwner>> result;
            for (int i = 0; i < 100; i++)
            {
                result.push_back(std::make_shared<BufferOwner>(FreeBuffers));
            }

            return result;
        }

        static void Send(std::list<BufferOwner>& buffer)
        {
        }


        IConnection* CreateConnection(const ConnectionAddr& addr)
        {
            return nullptr;
        }

        IConnection* AcceptConnection()
        {
            return nullptr;
        }


        void FastTransportContext::Run()
        {
            while (true)
            {
                std::list<std::shared_ptr<BufferOwner>> recvBuffers = Recv();

                for (auto& recv : recvBuffers)
                {
                    HeaderBuffer header = recv->GetHeader();
                    AddrBuffer addr = recv->GetAddr();
                    if (!header.IsValid())
                    {
                        continue;
                    }

                    auto connection = _connections.find(ConnectionKey(addr.GetAddr(), header.GetConnectionID()));

                    if (connection != _connections.end())
                    {
                        connection->second->OnRecvPackets(recv);
                    }
                    else
                    {
                        Connection* connection = _listen.Listen(recv);
                        if (connection)
                            _connections.insert({ connection->GetConnectionKey(), connection });
                        //ListenSockets
                    }

                }


                std::list<BufferOwner> buffers;
                Send(buffers);

            }
        }
    }
}
