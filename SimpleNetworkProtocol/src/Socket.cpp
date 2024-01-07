#include "Socket.hpp"


#include <algorithm>
#include <ctime>
#include <functional>
#include <utility>

#ifdef __linux__
#include <cassert>
#include <iterator>
#include <memory>
#include <netinet/udp.h>
#include <ranges>
#include <sys/socket.h>
#include <unordered_map>

#include "IPacket.hpp"
#include "OutgoingPacket.hpp"

static constexpr size_t ControlMessageSpace { CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(struct in_pktinfo)) + CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(sizeof(uint16_t)) };

#if defined(UDP_MAX_SEGMENTS) and UDP_MAX_SEGMENTS != 64
#error "Unexpected UDP_MAX_SEGMENTS size"
#endif

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
    result = setsockopt(_socket, SOL_UDP, UDP_SEGMENT, &GsoSize, sizeof(GsoSize));
    if (result != 0) {
        throw std::runtime_error("Socket: failed to set UDP_SEGMENT");
    }

    int gro = 1;
    result = setsockopt(_socket, IPPROTO_UDP, UDP_GRO, &gro, sizeof(gro));
    if (result != 0) {
        throw std::runtime_error("Socket: failed to set UDP_SEGMENT");
    }

    // Note for documentation: Using this feature requires using the fq queue discipline, which can be activated per interface using something like this:
    // tc qdisc add dev eth0 root fq
    int bytesPerSecond = 1024 * 1024 * 1000;
    result = setsockopt(_socket, SOL_SOCKET, SO_MAX_PACING_RATE, &bytesPerSecond, sizeof(bytesPerSecond));
    if (result != 0) {
        throw std::runtime_error("Socket: failed to set SO_MAX_PACING_RATE");
    }
#endif

    // bind socket to port
    if (bind(_socket, reinterpret_cast<const sockaddr*>(&_address.GetAddr()), sizeof(sockaddr)) != 0) { // NOLINT
        throw std::runtime_error("Socket: failed to bind");
    }
}

int Socket::SendTo(std::span<const std::byte> buffer, const ConnectionAddr& addr) const
{
    return sendto(_socket, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<const struct sockaddr*>(&addr.GetAddr()), sizeof(sockaddr_in)); // NOLINT
}

uint32_t Socket::SendMsg(OutgoingPacket::List& packets, size_t index) const
{
    auto tranform = [index](OutgoingPacket& packet) {
        auto destination = packet.GetPacket()->GetDstAddr();
        destination.SetPort(destination.GetPort() + index);
        return std::pair<ConnectionAddr, std::reference_wrapper<IPacket::Ptr>> { destination, packet.GetPacket() };
    };

    auto insertPacket = [](std::unordered_map<ConnectionAddr, std::vector<std::reference_wrapper<IPacket::Ptr>>, ConnectionAddr::HashFunction>& map1, const std::pair<ConnectionAddr, std::reference_wrapper<IPacket::Ptr>>& value) {
        auto&& [address, packet] = value;
        map1[address].push_back(packet);
        return map1;
    };
    auto packetsByAddress = std::transform_reduce(packets.begin(), packets.end(), std::unordered_map<ConnectionAddr, std::vector<std::reference_wrapper<IPacket::Ptr>>, ConnectionAddr::HashFunction>(), insertPacket, tranform);

    std::vector<mmsghdr> headers;
    headers.reserve(packetsByAddress.size());

    struct IoCtrl {
        std::vector<char> ctrls;
        std::vector<iovec> iov;
    } __attribute__((aligned(64)));

    std::vector<IoCtrl> ioCtrls(packetsByAddress.size());

    headers = std::transform_reduce(
        packetsByAddress.begin(), packetsByAddress.end(), ioCtrls.begin(), headers,
        [](std::vector<mmsghdr> list, std::vector<mmsghdr> messages) {
            list.insert(list.end(), messages.begin(), messages.end());
            return list;
        },
        [](auto& arg, auto& ioctrl) {
            auto& [address, packets] = arg;

            std::vector<iovec>& iov = ioctrl.iov;
            std::vector<char>& ctrl = ioctrl.ctrls;
            iov.resize(packets.size());
            ctrl.resize(packets.size());

            std::vector<mmsghdr> messages;
            messages.reserve(packets.size() / UDPMaxSegments + 1);
            auto iovBegin = iov.begin();
            auto ctrlBegin = ctrl.begin();
            auto packetChunks = packets | std::views::chunk(std::min<size_t>(UDPMaxSegments, 65000 / GsoSize));
            for (const auto& packetChunk : packetChunks) {

                auto iovEnd = std::transform(packetChunk.begin(), packetChunk.end(), iovBegin, [](const IPacket::Ptr& packet) {
                    iovec iovec {
                        .iov_base = packet->GetElement().data(),
                        .iov_len = packet->GetElement().size(),
                    };
                    return iovec;
                });

                auto packetChunkSize = packetChunk.size();
                struct msghdr message = {
                    .msg_name = const_cast<void*>(static_cast<const void*>(&(address.GetAddr()))), //NOLINT(cppcoreguidelines-pro-type-const-cast)
                    .msg_namelen = sizeof(sockaddr),
                    .msg_iov = std::addressof(*iovBegin),
                    .msg_iovlen = packetChunkSize,
                    .msg_control = std::addressof(*ctrlBegin),
                    .msg_controllen = 0,
                };

                iovBegin += packetChunkSize;
                assert(iovBegin == iovEnd);
                ctrlBegin += packetChunkSize;
                messages.push_back(mmsghdr { .msg_hdr = message });
            }
            return messages;
        });

    int result = sendmmsg(_socket, headers.data(), headers.size(), /*MSG_CONFIRM*/0);
    if (result < std::ssize(headers)) {
        throw std::runtime_error("Not implemented");
    }

    size_t len = 0;
    for (auto& header : headers) {
        len += header.msg_len;
    }

    uint32_t msgLength = 0;
    for (const auto& header : headers) {
        msgLength += header.msg_len;
    }

    return msgLength;
}

int Socket::RecvFrom(std::span<std::byte> buffer, ConnectionAddr& connectionAddr) const
{
    socklen_t len = sizeof(sockaddr_storage);
    sockaddr_storage addr {};
    int receivedBytes = recvfrom(_socket, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<struct sockaddr*>(&addr), &len); // NOLINT
    connectionAddr = ConnectionAddr(addr);
    return receivedBytes;
}

[[nodiscard]] IPacket::List Socket::RecvMsg(IPacket::List& packets, size_t index) const
{
    IPacket::List freePackets;
    auto packetChunks = packets | std::views::chunk(std::min<size_t>(UDPMaxSegments, 64));
    std::vector<iovec> iov(packets.size());
    std::vector<char> control(ControlMessageSpace);
    std::vector<sockaddr_storage> addresses(packets.size());
    auto iovBegin = iov.begin();
    auto controlBegin = control.begin();
    auto addressBegin = addresses.begin();
    auto messageRange = packetChunks | std::views::transform([&iovBegin, &controlBegin, &addressBegin](const auto& packetChunk) {
        auto iovRange = packetChunk | std::views::transform([](const IPacket::Ptr& packet) {
            return iovec {
                .iov_base = packet->GetElement().data(),
                .iov_len = packet->GetElement().size(),
            };
        });

        const auto& [_, iovEnd] = std::ranges::copy(iovRange.begin(), iovRange.end(), iovBegin);
        auto iovSize = iovEnd - iovBegin;
        if (iovSize != UDPMaxSegments) {
            assert(iovSize < UDPMaxSegments);
            return mmsghdr { .msg_hdr = msghdr {} };
        }

        msghdr message {
            .msg_name = std::addressof(*addressBegin),
            .msg_namelen = sizeof(sockaddr),
            .msg_iov = std::addressof(*iovBegin),
            .msg_iovlen = UDPMaxSegments,
            .msg_control = std::addressof(*controlBegin),
            .msg_controllen = UDPMaxSegments,
            .msg_flags = 0,
        };

        controlBegin += UDPMaxSegments;
        iovBegin += UDPMaxSegments;

        return mmsghdr { .msg_hdr = message };
    });

    std::vector<mmsghdr> messages;
    messages.reserve(10); // TODO: preallocate memory
    std::ranges::copy(messageRange.begin(), messageRange.end(), std::back_inserter(messages));
    timespec time {
        .tv_sec = 1,
    };

    auto messagesSize = (messages.back().msg_hdr.msg_iovlen != 0u) ? messages.size() : messages.size() - 1;
    int result = recvmmsg(_socket, messages.data(), messagesSize, MSG_WAITFORONE, &time);
    if (result < 0) {
        assert(false);
        freePackets.splice(std::move(packets));
        return freePackets;
    }

    auto packetIterator = packets.begin();
    for (const mmsghdr& message : messages | std::views::take(result)) {
        if (((static_cast<std::size_t>(message.msg_hdr.msg_flags)) & (static_cast<std::size_t>(MSG_CTRUNC) | MSG_TRUNC)) != 0) {
            throw std::runtime_error("Not implemented");
        }

        size_t packetNumber = (message.msg_len + GsoSize - 1) / GsoSize;

        for (size_t i = 0; i < packetNumber; i++) {

            ConnectionAddr srcAddress(*static_cast<sockaddr_storage*>(message.msg_hdr.msg_name));
            srcAddress.SetPort(srcAddress.GetPort() - index);
            (*packetIterator)->SetAddr(srcAddress);

            packetIterator++;
        }

        // TODO: Can splice several packets at once
        for (size_t i = packetNumber; i < message.msg_hdr.msg_iovlen; i++) {
            {
                freePackets.push_back(std::move(*packetIterator));
                packetIterator = packets.erase(packetIterator);
            }
        }
    }

    while (packetIterator != packets.end()) {
        freePackets.push_back(std::move(*packetIterator));
        packetIterator = packets.erase(packetIterator);
    }

    return freePackets;
}

} // namespace FastTransport::Protocol
