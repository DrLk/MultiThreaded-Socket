#pragma once

#include <unordered_map>
#include <memory>

#include "Connection.h"
/*#include <list>
#include <unordered_map>
#include <exception>
#include <algorithm>
#include "BufferOwner.h"*/

namespace FastTransport
{
    namespace Protocol
    {
        class FastTransport
        {
        public:
            FastTransport()
            {

            }

            void Run()
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

                }
            }


            static BufferOwner::BufferType FreeBuffers;

            std::list<std::shared_ptr<BufferOwner>> Recv()
            {

                std::list<std::shared_ptr<BufferOwner>> result;
                for (int i = 0; i < 100; i++)
                {
                    result.push_back(std::make_shared<BufferOwner>(FreeBuffers));
                }

                return result;
            }

            void Send(std::list<BufferOwner>& buffer)
            {

            }

        private:
            std::unordered_map<ConnectionIDType, Connection> _connections;
        };



    }
}
