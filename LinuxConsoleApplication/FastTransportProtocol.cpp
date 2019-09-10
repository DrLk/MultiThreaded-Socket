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


        void FastTransportContext::Run()
        {
            while (true)
            {
                std::list<std::shared_ptr<BufferOwner>> recvBuffers = Recv();

                for (auto& recv : recvBuffers)
                {
                    HeaderBuffer header = recv->GetHeader();
                    if (!header.IsValid())
                    {
                        continue;
                    }

                    auto connection = _connections.find(header.GetConnectionID());

                    if (connection != _connections.end())
                    {
                        connection->second.OnRecvPackets(recv);

                    }
                    else
                    {
                        //ListenSockets
                    }

                }


                std::list<BufferOwner> buffers;
                Send(buffers);

            }
        }
    }
}
