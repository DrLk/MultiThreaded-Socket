#pragma once

#ifdef WIN32
#include <Ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <cstring>
#include <string>
#include <stdexcept>


namespace FastTransport
{
    namespace Protocol
    {

        class ConnectionAddr
        {
        public:
            ConnectionAddr()
            {
                std::memset(&_storage, 0, sizeof(_storage));
            };

            ConnectionAddr(const std::string& addr, unsigned short port) 
            {
                std::memset(&_storage, 0, sizeof(_storage));
                if (inet_pton(AF_INET, addr.c_str(), &((sockaddr_in*)&_storage)->sin_addr))
                {
                    _storage.ss_family = AF_INET;
                    ((sockaddr_in*)&_storage)->sin_port = htons(port);
                }
                else if (inet_pton(AF_INET6, addr.c_str(), &((sockaddr_in6*)&_storage)->sin6_addr))
                {
                    _storage.ss_family = AF_INET6;
                    ((sockaddr_in6*)&_storage)->sin6_port = htons(port);
                }
            }

            ConnectionAddr(const ConnectionAddr& that) : _storage(that._storage) { }
            bool operator==(const ConnectionAddr& that) const
            {
                return std::memcmp(&_storage, &that._storage, sizeof(sockaddr_storage)) == 0;
            }

            unsigned int GetPort() const
            {
                return ntohs(((sockaddr_in*)&_storage)->sin_port);
            }
        private:
            sockaddr_storage _storage;
        };

    }
}
