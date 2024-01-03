#include "Socket.hpp"

#include "IPacket.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <unordered_map>
#include <utility>

#ifdef __linux__
#include <map>
#include <netinet/udp.h>
#include <sys/socket.h>
#endif

namespace FastTransport::Protocol {
void Socket::Init()
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

#ifdef __linux__
    int gso_size = 400;
    result = setsockopt(_socket, SOL_UDP, UDP_SEGMENT, &gso_size, sizeof(gso_size));
    if (result != 0) {
        throw std::runtime_error("Socket: failed to set UDP_SEGMENT");
    }

    int gro = 1;
    result = setsockopt(_socket, IPPROTO_UDP, UDP_GRO, &gro, sizeof(gro));
    if (result != 0) {
        throw std::runtime_error("Socket: failed to set UDP_SEGMENT");
    }
#endif

    // bind socket to port
    if (bind(_socket, reinterpret_cast<const sockaddr*>(&_address.GetAddr()), sizeof(sockaddr)) != 0) { // NOLINT
        throw std::runtime_error("Socket: failed to bind");
    }
}

void Socket::Init2()
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

    /* #ifdef __linux__ */
    /*     int gso_size = 400; */
    /*     result = setsockopt(_socket, SOL_UDP, UDP_SEGMENT, &gso_size, sizeof(gso_size)); */
    /*     if (result != 0) { */
    /*         throw std::runtime_error("Socket: failed to set UDP_SEGMENT"); */
    /*     } */
    /* #endif */

    // bind socket to port
    if (bind(_socket, reinterpret_cast<const sockaddr*>(&_address.GetAddr()), sizeof(sockaddr)) != 0) { // NOLINT
        throw std::runtime_error("Socket: failed to bind");
    }
}

int Socket::SendTo(std::span<const std::byte> buffer, const ConnectionAddr& addr) const
{
    return sendto(_socket, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<const struct sockaddr*>(&addr.GetAddr()), sizeof(sockaddr_in)); // NOLINT
}

int Socket::SendMsg(IPacket::List& packets, const ConnectionAddr& addr) const
{

    auto tranform = [](auto& packet) {
        return std::pair<int, std::reference_wrapper<IPacket::Ptr>> { packet->GetDstAddr().GetPort(), packet };
    };

    auto insertPacket = [](std::unordered_map<int, std::vector<std::reference_wrapper<IPacket::Ptr>>>& map1, const std::pair<int, std::reference_wrapper<IPacket::Ptr>>& value) {
        auto&& [address, packet] = value;
        map1[address].push_back(packet);
        return map1;
    };
    auto sortedPackets = std::transform_reduce(packets.begin(), packets.end(), std::unordered_map<int, std::vector<std::reference_wrapper<IPacket::Ptr>>>(), insertPacket, tranform);

    std::vector<mmsghdr> headers(sortedPackets.size());

    struct IoCtrl {
        std::vector<char> ctrls;
        std::vector<iovec> iov;
    } __attribute__((aligned(64)));

    std::vector<IoCtrl> ioCtrls(sortedPackets.size());

    std::transform(sortedPackets.begin(), sortedPackets.end(), ioCtrls.begin(), headers.begin(), [&addr](auto& arg, auto& ioctrl) {
        auto& [address, packets] = arg;

        std::vector<iovec>& iov = ioctrl.iov;
        std::vector<char>& ctrl = ioctrl.ctrls;
        iov.resize(packets.size());
        ctrl.resize(packets.size());

        std::transform(packets.begin(), packets.end(), std::begin(iov), [](const IPacket::Ptr& packet) {
            return iovec {
                .iov_base = packet->GetBuffer().data(),
                .iov_len = packet->GetBuffer().size(),
            };
        });

        struct msghdr message = {
            .msg_name = const_cast<void*>(static_cast<const void*>(&(addr.GetAddr()))),
            .msg_namelen = sizeof(sockaddr),
            .msg_iov = iov.data(),
            .msg_iovlen = iov.size(),
            .msg_control = ctrl.data(),
            .msg_controllen = 0,
        };
        return mmsghdr { .msg_hdr = message };
    });

    auto result = sendmmsg(_socket, headers.data(), headers.size(), MSG_ZEROCOPY);

    unsigned int msg_length = 0;
    for (const auto& header : headers) {
        msg_length += header.msg_len;
    }

    return msg_length;

    /* for (auto& [address, packets] : sortedPackets) { */
    /*     std::vector<iovec> iov; */
    /*     std::vector<char> ctrl; */
    /*     std::transform(packets.begin(), packets.end(), std::begin(iov), [](const IPacket::Ptr& packet) { */
    /*         iovec iov {}; */
    /*  */
    /*         iov.iov_base = packet->GetBuffer().data(); */
    /*         iov.iov_len = packet->GetBuffer().size(); */
    /*         return iov; */
    /*     }); */
    /*  */
    /*     struct msghdr message = { */
    /*         .msg_iov = iov.data(), */
    /*         .msg_iovlen = iov.size(), */
    /*         .msg_control = ctrl.data(), */
    /*         .msg_controllen = 0, */
    /*         .msg_name = const_cast<void*>(static_cast<const void*>(&(addr.GetAddr()))), */
    /*         .msg_namelen = sizeof(sockaddr), */
    /*     }; */
    /*     headers.push_back(mmsghdr { .msg_hdr = message }); */
    /* } */
    /*  */
    /* return sendmmsg(_socket, headers.data(), headers.size(), 0); */

    /* static constexpr size_t CtrlSize = (sizeof(struct cmsghdr) + sizeof(int)); */
    /* std::array<char, CtrlSize> ctrl {}; */
    /*  */
    /* static constexpr size_t Size = 10; */
    /* std::array<iovec, Size> iov2 {}; */
    /*  */
    /* std::transform(packets.begin(), packets.end(), std::begin(iov2), [](const IPacket::Ptr& packet) { */
    /*     iovec iov {}; */
    /*  */
    /*     iov.iov_base = packet->GetBuffer().data(); */
    /*     iov.iov_len = packet->GetBuffer().size(); */
    /*     return iov; */
    /* }); */
    /*  */
    /* struct msghdr message2 = { */
    /*     .msg_iov = iov2.data(), */
    /*     .msg_iovlen = std::min(Size, packets.size()), */
    /*     .msg_control = ctrl.data(), */
    /*     .msg_controllen = 0, */
    /* }; */
    /* message2.msg_name = const_cast<void*>(static_cast<const void*>(&(addr.GetAddr()))); */
    /* message2.msg_namelen = sizeof(sockaddr); */
    /*  */
    /* std::vector<mmsghdr> messages; */
    /*  */
    /* messages[0].msg_hdr = message2; */
    /*  */
    /* sendmmsg(_socket, messages.data(), messages.size(), 0); */
    /*  */
    /* return sendmsg(_socket, &message2, 0); */
}

int Socket::RecvFrom(std::span<std::byte> buffer, ConnectionAddr& connectionAddr) const
{
    socklen_t len = sizeof(sockaddr_storage);
    sockaddr_storage addr {};
    int receivedBytes = recvfrom(_socket, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<struct sockaddr*>(&addr), &len); // NOLINT
    connectionAddr = ConnectionAddr(addr);
    return receivedBytes;
}

[[nodiscard]] int Socket::RecvMsg(IPacket::List& packets, ConnectionAddr& connectionAddr) const
{
    std::vector<iovec> iov(packets.size());
    std::transform(packets.begin(), packets.end(), iov.begin(), [](const IPacket::Ptr& packet) {
        return iovec {
            .iov_base = packet->GetElement().data(),
            .iov_len = packet->GetElement().size(),
        };
    });

    sockaddr_storage addr {};
    std::array<char, ControlMessageSpace> control {};
    msghdr message {};
    message.msg_iov = iov.data();
    message.msg_iovlen = iov.size();
    message.msg_name = &addr;
    message.msg_namelen = sizeof(sockaddr);
    message.msg_control = control.data();
    message.msg_controllen = control.size();
    message.msg_flags = 0;
    size_t result = recvmsg(_socket, &message, /*MSG_PEEK*/ 0);
    connectionAddr = ConnectionAddr(addr);

    if ((message.msg_flags & MSG_CTRUNC) == 0) {
        int i = 0;
        i++;
        i = errno;
    }
    if (message.msg_namelen > 0) {
        int i = 9;
        i++;
    }

    return static_cast<int>(result);
}

} // namespace FastTransport::Protocol
