#pragma once



namespace FastTransport
{
    namespace Protocol
    {

        class ConnectionAddr
        {
        public:
            ConnectionAddr() { }
            ConnectionAddr(const ConnectionAddr& that) { }
            bool operator==(const ConnectionAddr that) const
            {
                throw std::runtime_error("Not implemented");
            }

            unsigned int GetPort() const
            {
                throw std::runtime_error("Not Implemented");
            }

        };

    }
}