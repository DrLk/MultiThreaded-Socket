#include "Socket.hpp"
#include "IPacket.hpp"

#include <algorithm>
#include <unordered_map>

#ifndef POSIX
/* #include <linux/udp.h> */
#include <map>
#include <netinet/udp.h>
#include <numeric>
#include <string>
#include <sys/socket.h>
/* #include <sys/socket.h> */
#endif

namespace FastTransport::Protocol {
void Socket::Init()
{

    {
        // create a UDP socket
        _socket = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
        if (_socket == INVALID_SOCKET) {
            throw std::runtime_error("socket");
        }

#ifdef WIN32
        u_long mode = 1; // 1 to enable non-blocking socket
        ioctlsocket(_socket, FIONBIO, &mode); // NOLINT
#elif __APPLE__
        fcntl(_socket, F_SETFL, O_NONBLOCK); // NOLINT
#else
        uint32_t mode = 1; // 1 to enable non-blocking socket
        ioctl(_socket, FIONBIO, &mode); // NOLINT
#endif

        const int bufferSize = 10 * 1024 * 1024;
        int result = setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&bufferSize), sizeof(bufferSize)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        if (result != 0) {
            throw std::runtime_error("Socket: failed to set SO_RCVBUF");
        }

        result = setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&bufferSize), sizeof(bufferSize)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        if (result != 0) {
            throw std::runtime_error("Socket: failed to set SO_SNDBUF");
        }
        /* int gso_size = ETH_DATA_LEN - sizeof(struct ipv6hdr) - sizeof(struct udphdr); */
        int gso_size = 1000;
        result = setsockopt(_socket, SOL_UDP, UDP_SEGMENT, &gso_size, sizeof(gso_size));
        if (result != 0) {
            throw std::runtime_error("Socket: failed to set UDP_SEGMENT");
        }

        // bind socket to port
        if (bind(_socket, reinterpret_cast<const sockaddr*>(&_address.GetAddr()), sizeof(sockaddr)) != 0) { // NOLINT
            throw std::runtime_error("Socket: failed to bind");
        }
    }
}

int Socket::SendTo(std::span<const std::byte> buffer, const ConnectionAddr& addr) const
{
    return sendto(_socket, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<const struct sockaddr*>(&addr.GetAddr()), sizeof(sockaddr_in)); // NOLINT
}

size_t Socket::SendMsg(IPacket::List& packets, const ConnectionAddr& addr) const
{
    static constexpr size_t CtrlSize = (sizeof(struct cmsghdr) + sizeof(int));
    std::array<char, CtrlSize> ctrl {};

    static constexpr size_t Size = 10;
    std::array<iovec, Size> iov {};

    std::unordered_map<int, std::vector<IPacket::Ptr>> map {};
    auto tranform = [](auto& packet) {
        return std::pair<int, IPacket::Ptr> { packet->GetDstAddr().GetPort(), std::move(packet)};
    };

    auto mergeMaps = [](std::unordered_map<int, std::vector<IPacket::Ptr>>& map1, std::pair<int, IPacket::Ptr>&& value) {
        map1[value.first].push_back(std::move(std::move(value.second)));
        std::unordered_map<int, std::vector<IPacket::Ptr>> result;
        /* result.insert(map1.begin(), map1.end()); */
        return result;
    };
    std::transform_reduce(packets.begin(), packets.end(), std::unordered_map<int, std::vector<IPacket::Ptr>>(), mergeMaps, tranform);

    std::transform(packets.begin(), packets.end(), std::begin(iov), [](const IPacket::Ptr& packet) {
        iovec iov {};

        iov.iov_base = packet->GetBuffer().data();
        iov.iov_len = packet->GetBuffer().size();
        return iov;
    });

    struct msghdr message = {
        .msg_iov = iov.data(),
        .msg_iovlen = std::min(Size, packets.size()),
        .msg_control = ctrl.data(),
        .msg_controllen = 0,
    };
    message.msg_name = const_cast<void*>(static_cast<const void*>(&(addr.GetAddr())));
    message.msg_namelen = sizeof(sockaddr);

    std::vector<mmsghdr> messages;

    messages[0].msg_hdr = message;

    sendmmsg(_socket, messages.data(), messages.size(), 0);

    return sendmsg(_socket, &message, 0);
}

} // namespace FastTransport::Protocol
