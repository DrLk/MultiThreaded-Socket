#pragma once


namespace FastTransport
{
    namespace Protocol
    {

        class ConnectionAddr
        {
        public:
            bool operator==(const ConnectionAddr that) const
            {
                throw std::runtime_error("Not implemented");
            }

        };

    }
}
