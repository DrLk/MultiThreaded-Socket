#include "ConnectionWriter.hpp"

#include <functional>
#include <thread>
#include <utility>

namespace FastTransport::Protocol {
ConnectionWriter::ConnectionWriter(std::stop_token stop, const IConnection::Ptr& connection, IPacket::List&& packet)
    : _connection(connection)
    , _packets(std::move(packet))
    , _packet(_packets.begin())
      ,_stop(std::move(stop))
    , _sendThread(SendThread, std::ref(*this), std::ref(*connection))
{
}

ConnectionWriter::~ConnectionWriter()
{
    Flush();
}

} // namespace FastTransport::Protocol
