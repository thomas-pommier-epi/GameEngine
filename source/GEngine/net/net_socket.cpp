/*
** EPITECH PROJECT, 2024
** B-CPP-500-LYN-5-1-rtype-basile.fouquet (Workspace)
** File description:
** socket
*/

#include "GEngine/net/net_socket.hpp"
#include "GEngine/cvar/net.hpp"
#include "GEngine/net/net.hpp"
#include "GEngine/net/net_exception.hpp"
#include "GEngine/net/net_socket_error.hpp"
#include "GEngine/net/net_wait.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

#ifdef _WIN32

#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace Network {

#ifdef _WIN32
WSADATA ASocket::winsockdata;
#endif

ASocket::ASocket(ASocket &&other) {
    m_sock = other.m_sock;
    other.m_sock = -1;
#ifdef NET_USE_HANDLE
    m_handle = other.m_handle;
    other.m_handle = INVALID_HANDLE_VALUE;
#endif
}

ASocket &ASocket::operator=(ASocket &&other) {
    if (this != &other) {
        m_sock = other.m_sock;
        other.m_sock = -1;
#ifdef NET_USE_HANDLE
        m_handle = other.m_handle;
        other.m_handle = INVALID_HANDLE_VALUE;
#endif
    }
    return *this;
}

//////////////////////////////////////

void ASocket::initLibs(void) {
    static bool initialized = false;
    if (initialized)
        return;
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(1, 1), &winsockdata))
        return;
#endif
    initialized = true;
}

int ASocket::socketCloseAdv(bool shouldShutdown) {
    int status = 0;

    if (m_sock == -1)
        return 0;

#ifdef _WIN32
    if (shouldShutdown)
        status = shutdown(m_sock, SD_BOTH);
    if (status == 0)
        status = closesocket(m_sock);
#else
    if (shouldShutdown)
        status = shutdown(m_sock, SHUT_RDWR);
    if (status == 0)
        status = close(m_sock);
#endif

#ifdef NET_USE_HANDLE
    if (m_handle != INVALID_HANDLE_VALUE)
        status = CloseHandle(m_handle) ? 0 : -1;
#endif
    NET::getWaitHandler().removeSocketPool(*this);

    return status;
}

#ifdef NET_USE_HANDLE
void ASocket::createHandle(void) {
    m_handle = WSACreateEvent();
    if (WSAEventSelect(m_sock, m_handle, FD_CONNECT | FD_ACCEPT | FD_READ | FD_CLOSE) != 0)
        throw SocketException("WSAEventSelect");
}
#endif

//////////////////////////////////////

ANetSocket::ANetSocket(ANetSocket &&other)
    : ASocket(std::move(other)) {
    m_port = other.m_port;
}

ANetSocket &ANetSocket::operator=(ANetSocket &&other) {
    if (this != &other)
        m_port = other.m_port;
    ASocket::operator=(std::move(other));
    return *this;
}

void ANetSocket::setBlocking(bool blocking) {
#ifdef _WIN32
    u_long mode = blocking ? 0 : 1;
    ioctlsocket(m_sock, FIONBIO, &mode);
#else
    int flags = fcntl(m_sock, F_GETFL, 0);
    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    fcntl(m_sock, F_SETFL, flags);
#endif
}

bool ANetSocket::isBlocking(void) const {
#ifdef _WIN32
    u_long mode;
    ioctlsocket(m_sock, FIONBIO, &mode);
    return mode == 0;
#else
    int flags = fcntl(m_sock, F_GETFL, 0);
    return (flags & O_NONBLOCK) == 0;
#endif
}

void ANetSocket::translateAutomaticAddressing(struct sockaddr_storage &addr_storage, uint16_t port, bool ipv6) {
    if (!ipv6) {
        struct sockaddr_in *addr = reinterpret_cast<struct sockaddr_in *>(&addr_storage);

        addr->sin_addr.s_addr = htonl(INADDR_ANY);
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
    } else {
        struct sockaddr_in6 *addr = reinterpret_cast<struct sockaddr_in6 *>(&addr_storage);

        addr->sin6_addr = IN6ADDR_ANY_INIT;
        addr->sin6_family = AF_INET6;
        addr->sin6_port = htons(port);
    }
}

/***********************************************/

SocketTCPMaster::SocketTCPMaster(const IP &ip, uint16_t port) {
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == -1)
        throw NetException("(TCP) Failed to create socket", EL_ERR_SOCKET);

    unsigned int opt = 1;
    if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&opt), sizeof(opt)) < 0)
        throw SocketException("(TCP) Failed to set socket options");

    m_port = port;
    struct sockaddr_in address;
    std::memcpy(&address, &ip.addr, sizeof(struct sockaddr_in));

    address.sin_port = htons(port);

    if (bind(m_sock, (sockaddr *)&address, ip.type == AT_IPV6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in)) < 0)
        throw SocketException("(TCP) Failed to bind socket");

    if (listen(m_sock, MAX_LISTEN) < 0)
        throw SocketException("(TCP) Failed to listen on socket");

    NET::getWaitHandler().addSocketPool(*this);
}

SocketTCPMaster::SocketTCPMaster(uint16_t port, bool ipv6) {
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == -1)
        throw NetException("(TCP) Failed to create socket", EL_ERR_SOCKET);

    unsigned int opt = 1;
    if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&opt), sizeof(opt)) < 0)
        throw SocketException("(TCP) Failed to set socket options");

    m_port = port;
    struct sockaddr_storage address = {0};
    translateAutomaticAddressing(address, port, ipv6);

    if (bind(m_sock, (sockaddr *)&address, ipv6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in)) < 0)
        throw SocketException("(TCP) Failed to bind socket");

    if (listen(m_sock, MAX_LISTEN) < 0)
        throw SocketException("(TCP) Failed to listen on socket");

    NET::getWaitHandler().addSocketPool(*this);
}

SocketTCPMaster::~SocketTCPMaster() {
    socketClose();
}

SocketTCPMaster::SocketTCPMaster(SocketTCPMaster &&other)
    : ANetSocket(std::move(other)) {
}

SocketTCPMaster &SocketTCPMaster::operator=(SocketTCPMaster &&other) {
    if (this != &other)
        ANetSocket::operator=(std::move(other));
    return *this;
}

SocketTCP SocketTCPMaster::accept(UnknownAddress &unkwAddr) const {
    return std::move(SocketTCP(*this, unkwAddr));
}

/***********************************************/

SocketTCP::SocketTCP(const SocketTCPMaster &socketMaster, UnknownAddress &unkwAddr, bool block) {
    m_sock = accept(socketMaster.getSocket(), unkwAddr.getAddr(), &unkwAddr.getLen());
    setBlocking(block);
    if (m_sock == -1)
        throw NetException("Failed to accept connectionEL_");

    unkwAddr.updateType();
    m_port = socketMaster.getPort();
    m_notReady = false;
    NET::getWaitHandler().addSocketPool(*this);
}

SocketTCP::SocketTCP(const AddressV4 &addr, uint16_t tcpPort, bool block) {
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setBlocking(block);
    if (m_sock == -1)
        throw NetException("(TCP) Failed to create socket", EL_ERR_SOCKET);

    m_port = tcpPort;
    struct sockaddr_in address;

    addr.toSockAddr(reinterpret_cast<struct sockaddr *>(&address));
    std::memcpy(&address, (struct sockaddr *)&address, sizeof(address));

    address.sin_port = htons(m_port);

    if (connect(m_sock, (sockaddr *)&address, sizeof(address)) < 0) {
        int e = socketError;
        if (e == WSAEINPROGRESS || e == WSAEWOULDBLOCK) {
            NET::getWaitHandler().addSocketPool(*this);
            return;
        }
        socketClose();
        throw SocketException("can't connect tcp");
    }

    m_notReady = false;
    NET::getWaitHandler().addSocketPool(*this);
}

SocketTCP::SocketTCP(const AddressV6 &addr, uint16_t tcpPort, bool block) {
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setBlocking(block);
    if (m_sock == -1)
        throw NetException("(TCP) Failed to create socket", EL_ERR_SOCKET);

    m_port = tcpPort;
    struct sockaddr_in6 address;

    addr.toSockAddr(reinterpret_cast<struct sockaddr *>(&address));
    std::memcpy(&address, (struct sockaddr *)&address, sizeof(address));

    address.sin6_port = htons(m_port);

    if (connect(m_sock, (sockaddr *)&address, sizeof(address)) < 0) {
        int e = socketError;
        if (e == WSAEINPROGRESS || e == WSAEWOULDBLOCK) {
            NET::getWaitHandler().addSocketPool(*this);
            return;
        }
        socketClose();
        throw SocketException("can't connect tcp");
    }

    m_notReady = false;
    NET::getWaitHandler().addSocketPool(*this);
}

SocketTCP::~SocketTCP() {
    socketClose();
}

SocketTCP::SocketTCP(SocketTCP &&other)
    : ANetSocket(std::move(other)) {
}

SocketTCP &SocketTCP::operator=(SocketTCP &&other) {
    if (this != &other)
        ANetSocket::operator=(std::move(other));
    return *this;
}

bool SocketTCP::send(const TCPMessage &msg) const {
    /* TODO : here, we guess the size is enough to
    send, should we break it here or before ? */

    TCPSerializedMessage sMsg;
    msg.getSerialize(sMsg);

    return sendReliant(&sMsg, msg.getSize(), 0) != 0; // if it did not block
}

void SocketTCP::receive(TCPMessage &msg, TCPSerializedMessage &sMsg, size_t &recvSz) const {
    auto ptrMsg = reinterpret_cast<char *>(&sMsg);

    if (recvSz < sizeof(HeaderSerializedMessage)) {
        recvSz = receiveReliant(reinterpret_cast<TCPSerializedMessage *>(ptrMsg + recvSz),
                                sizeof(HeaderSerializedMessage) - recvSz, 0, recvSz);
        if (sMsg.curSize > MAX_TCP_MSGLEN)
            throw SocketException("Message too big");
    }
    if (recvSz != sMsg.curSize) {
        recvSz = receiveReliant(reinterpret_cast<TCPSerializedMessage *>(ptrMsg + recvSz),
                                CF_NET_MIN(sMsg.curSize + 1 - (recvSz) + sizeof(HeaderSerializedMessage),
                                           sizeof(TCPSerializedMessage) - recvSz),
                                0, recvSz);
    }

    msg.setSerialize(sMsg);

    /* reset tmp buffer */
    sMsg = TCPSerializedMessage();
    recvSz = 0;
}

bool SocketTCP::sendPartial(const TCPMessage &msg, size_t sizeToSend, size_t &offset) const {
    TCPSerializedMessage sMsg;
    msg.getSerialize(sMsg);

    size_t nowOffset = CF_NET_MIN(offset, msg.getSize() + sizeof(TCPSerializedMessage) - MAX_TCP_MSGLEN);
    sizeToSend = CF_NET_MIN(msg.getSize() + nowOffset, CF_NET_MIN(sizeToSend, MAX_TCP_MSGLEN));

    size_t sent = sendReliant(&sMsg, sizeToSend, offset);
    offset += sent;
    return sent != 0;
}

bool SocketTCP::receivePartial(TCPSerializedMessage &sMsg, size_t size, size_t &offset) const {
    auto ptrMsg = reinterpret_cast<char *>(&sMsg);
    size_t recvSz = 0;

    if (offset < sizeof(HeaderSerializedMessage)) { /* new message */
        recvSz = receiveReliant(reinterpret_cast<TCPSerializedMessage *>(ptrMsg), sizeof(HeaderSerializedMessage),
                                offset, recvSz);
        offset += recvSz;
        if (offset < sizeof(HeaderSerializedMessage))
            return false; /* the message is too small, can't proceed */
    }

    size_t nowOffset = CF_NET_MIN(offset, sMsg.curSize + sizeof(TCPSerializedMessage) - MAX_TCP_MSGLEN);

    recvSz = receiveReliant(reinterpret_cast<TCPSerializedMessage *>(ptrMsg), sizeof(HeaderSerializedMessage),
                            nowOffset, recvSz);
    offset += recvSz;
    return true;
}

size_t SocketTCP::receiveReliant(TCPSerializedMessage *buffer, size_t size, size_t offset, size_t &recvSz) const {
    size_t receivedTotal = 0;

    while (receivedTotal < size) {
        auto received =
            ::recv(m_sock, reinterpret_cast<char *>(buffer + offset + receivedTotal), size - receivedTotal, 0);
        if (received == 0)
            throw SocketDisconnected();
        if (received < 0) {
            if (socketError == WSAECONNRESET)
                throw SocketDisconnected();
            recvSz += receivedTotal;
            throw SocketException(socketError);
        }
        receivedTotal += received;
    }
    return receivedTotal;
}

/* THIS IS THE ALL MESSAGE, NOT THE JUST DATA */
size_t SocketTCP::sendReliant(const TCPSerializedMessage *msg, size_t msgDataSize, size_t offset) const {
    size_t sentTotal = 0;
    msgDataSize += sizeof(TCPSerializedMessage) - MAX_TCP_MSGLEN;

    while (sentTotal < msgDataSize) {
        auto sent =
            ::send(m_sock, reinterpret_cast<const char *>(msg + offset + sentTotal), msgDataSize - sentTotal, 0);
        if (sent < 0) {
            int e = socketError;
            if (e == WSAECONNRESET)
                throw SocketDisconnected();
            if (e == WSAEWOULDBLOCK || e == WSATRY_AGAIN)
                return sentTotal;
            throw SocketException(e);
        }
        if (sent == 0)
            throw SocketDisconnected();
        sentTotal += sent;
    }
    return sentTotal;
}

/***********************************************/

SocketUDP::~SocketUDP() {
    socketClose();
}

void SocketUDP::init(bool block, uint16_t port) {
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock == -1)
        throw NetException("(UDP) Failed to create socket", EL_ERR_SOCKET);

    m_port = port;
    setBlocking(block);

#ifdef NET_DONT_FRAG
#if defined(__FreeBSD__) || defined(__APPLE__)
    if (setsockopt(m_sock, IPPROTO_IP, IP_DONTFRAG, &opt, sizeof(opt)))
#else
    if (setsockopt(m_sock, IPPROTO_IP, IP_MTU_DISCOVER, &opt, sizeof(opt)))
#endif
        throw NetException("(UDP) Failed to set socket options (IP_DONTFRAG)", EL_ERR_SOCKET);
#endif
}

SocketUDP::SocketUDP(const IP &ip, uint16_t port, bool block) {
    init(block, port);

    struct sockaddr_in address;
    std::memcpy(&address, &ip.addr, sizeof(address));

    address.sin_port = htons(port);

    if (bind(m_sock, (sockaddr *)&address, ip.type == AT_IPV6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in)) < 0)
        throw SocketException("(UDP) Failed to bind socket");

    NET::getWaitHandler().addSocketPool(*this);
}

SocketUDP::SocketUDP(uint16_t port, bool ipv6, bool block) {
    init(block, port);

    struct sockaddr_storage address = {0};
    translateAutomaticAddressing(address, port, ipv6);

    if (bind(m_sock, (sockaddr *)&address, ipv6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in)) < 0)
        throw SocketException("(UDP) can't bind udp (only port)");

    NET::getWaitHandler().addSocketPool(*this);
}

SocketUDP::SocketUDP(SocketUDP &&other)
    : ANetSocket(std::move(other)) {
}
SocketUDP &SocketUDP::operator=(SocketUDP &&other) {
    if (this != &other)
        ANetSocket::operator=(std::move(other));
    return *this;
}

size_t SocketUDP::send(const UDPMessage &msg, const Address &addr) const {
    /*
        BY THE WAY, m_sock is the same for EVERY CLASS, i don't put it on static
        because it's inherited
    */

    int simval = CVar::net_send_dropsim.getIntValue();
    if (simval > 0) {
        if (rand() < (int)(((double)RAND_MAX) / 100.0 * (double)simval))
            return 0; // drop this packet
    }

    UDPSerializedMessage sMsg;
    size_t size = msg.getSize();
    struct sockaddr_storage sockaddr = {0};
    msg.getSerialize(sMsg);

    addr.toSockAddr(reinterpret_cast<struct sockaddr *>(&sockaddr));
    auto sent =
        sendto(m_sock, reinterpret_cast<const char *>(&sMsg), size + sizeof(UDPSerializedMessage) - MAX_UDP_MSGLEN, 0,
               (struct sockaddr *)&sockaddr, sizeof(struct sockaddr));
    if (sent < 0) {
        int e = socketError;
        if (e == WSAEWOULDBLOCK || e == WSATRY_AGAIN)
            return 0;
        throw NetException("Failed to send message (invalid , EL_ERR_SOCKETaddress)");
    }
    return sent;
}

bool SocketUDP::receive(struct sockaddr *addr, UDPSerializedMessage &data, socklen_t *len) const {
    size_t recv = recvfrom(m_sock, reinterpret_cast<char *>(&data), sizeof(UDPSerializedMessage), 0, addr, len);
    int64_t recvStatus = recv;

    int simval = CVar::net_recv_dropsim.getIntValue();
    if (simval > 0) {
        if (rand() < (int)(((double)RAND_MAX) / 100.0 * (double)simval))
            return false; // drop this packet
    }

    // checking wouldblock etc... select told us to read so it's not possible
    if (recvStatus < 0) {
        int e = socketError;
        if (e == WSAEWOULDBLOCK || e == WSATRY_AGAIN || e == WSAECONNRESET)
            return false;

        throw SocketException(e);
    }

    if (recv < sizeof(UDPSerializedMessage) - MAX_UDP_MSGLEN)
        return false;
    return true;
}

bool SocketUDP::receiveV4(UDPMessage &msg, AddressV4 &ip) const {
    UDPSerializedMessage sMsg;
    struct sockaddr_in addr = {0};
    socklen_t len = sizeof(addr);

    if (!receive(reinterpret_cast<struct sockaddr *>(&addr), sMsg, &len))
        return false;
    msg.setSerialize(sMsg);

    ip = AddressV4(AT_IPV4, ntohs(addr.sin_port), ntohl(addr.sin_addr.s_addr));
    return true;
}

/* TODO : one day i might directly pack the udpmessage at this point */
bool SocketUDP::receiveV6(UDPMessage &msg, AddressV6 &ip) const {
    UDPSerializedMessage sMsg;
    struct sockaddr_in6 addr;
    socklen_t len = sizeof(addr);

    if (!receive(reinterpret_cast<struct sockaddr *>(&addr), sMsg, &len))
        return false;
    msg.setSerialize(sMsg);

    ip = AddressV6(AT_IPV6, ntohs(addr.sin6_port), addr.sin6_addr, addr.sin6_scope_id);
    return true;
}

int SocketUDP::setInterface(const IP &ip) {
    struct sockaddr_storage address;
    std::memcpy(&address, &ip.addr, sizeof(address));

#ifdef _WIN32
    if (ip.type == AT_IPV6) {
        unsigned int index = reinterpret_cast<struct sockaddr_in6 *>(&address)->sin6_scope_id;
        if (setsockopt(m_sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&index, sizeof(index)) < 0)
            throw NetException("(UDP) Failed to set socket options (IPV6_MULTICAST_IF)", EL_ERR_SOCKET);
    } else {
        struct in_addr addr = reinterpret_cast<struct sockaddr_in *>(&address)->sin_addr;
        if (setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&addr, sizeof(addr)) < 0)
            throw NetException("(UDP) Failed to set socket options (IP_MULTICAST_IF)", EL_ERR_SOCKET);
    }
#else
    if (ip.type == AT_IPV6) {
        struct in6_addr addr = reinterpret_cast<struct sockaddr_in6 *>(&address)->sin6_addr;
        if (setsockopt(m_sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &addr, sizeof(addr)) < 0)
            throw NetException("(UDP) Failed to set socket options (IPV6_MULTICAST_IF)", EL_ERR_SOCKET);
    } else {
        struct in_addr addr = reinterpret_cast<struct sockaddr_in *>(&address)->sin_addr;
        if (setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr)) < 0)
            throw NetException("(UDP) Failed to set socket options (IP_MULTICAST_IF)", EL_ERR_SOCKET);
    }
#endif
    return 0;
}

void SocketUDP::setPingResponse(void) {
#ifdef NET_USE_BROADCAST
    unsigned int opt = 1;
    if (setsockopt(m_sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)))
        throw NetException("(UDP) Failed to set socket options (SO_BROADCAST)", EL_ERR_SOCKET);
#else
    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_IPV4);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0)
        throw NetException("(UDP) Failed to set socket options (IP_ADD_MEMBERSHIP)", EL_ERR_SOCKET);
#endif
}

/*****************************************************/

SocketTCPMaster openSocketTcp(const IP &ip, uint16_t &wantedPort) {
    for (uint16_t i = 0; i < MAX_TRY_PORTS; i++) {
        try {
            return std::move(SocketTCPMaster(ip, wantedPort++));
        } catch (SocketException &e) {
            if (!e.shouldRetry() || i == MAX_TRY_PORTS - 1)
                throw e;
            continue;
        }
    }
    throw NetException("Failed to open TCP socket", EL_ERR_SOCKET);
}

SocketUDP openSocketUdp(const IP &ip, uint16_t &wantedPort) {
    for (uint16_t i = 0; i < MAX_TRY_PORTS; i++) {
        try {
            return std::move(SocketUDP(ip, wantedPort++));
        } catch (SocketException &e) {
            if (!e.shouldRetry() || i == MAX_TRY_PORTS - 1)
                throw e;
            continue;
        }
    }
    throw NetException("Failed to open UDP socket", EL_ERR_SOCKET);
}

SocketTCPMaster openSocketTcp(uint16_t &wantedPort, bool ipv6) {
    for (uint16_t i = 0; i < MAX_TRY_PORTS; i++) {
        try {
            return std::move(SocketTCPMaster(wantedPort++, ipv6));
        } catch (SocketException &e) {
            if (!e.shouldRetry() || i == MAX_TRY_PORTS - 1)
                throw e;
            continue;
        }
    }
    throw NetException("Failed to open TCP socket", EL_ERR_SOCKET);
}

SocketUDP openSocketUdp(uint16_t &wantedPort, bool ipv6) {
    for (uint16_t i = 0; i < MAX_TRY_PORTS; i++) {
        try {
            return std::move(SocketUDP(wantedPort++, ipv6));
        } catch (SocketException &e) {
            if (!e.shouldRetry() || i == MAX_TRY_PORTS - 1)
                throw e;
            continue;
        }
    }
    throw NetException("Failed to open UDP socket", EL_ERR_SOCKET);
}

} // namespace Network