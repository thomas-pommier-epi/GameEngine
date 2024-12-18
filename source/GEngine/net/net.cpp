/*
** EPITECH PROJECT, 2024
** B-CPP-500-LYN-5-1-rtype-basile.fouquet (Workspace)
** File description:
** net
*/

// udp : why  the chunks are 1400 bytes long?
// https://stackoverflow.com/questions/1098897/what-is-the-largest-safe-udp-packet-size-on-the-internet

// for tcp : reliable messages (chat, image upload/download, game changes (map
// update, changing connection state of client)) for udp : unreliable messages
// (snapshot deltas/(full because dummy is set to all 0, meaning delta = full),
// voip)

// open both UDP socket for ipv4 and ipv6 for the same class
// add multicast for ipv6 --> NOT PRIORITARY
// https://en.wikipedia.org/wiki/NACK-Oriented_Reliable_Multicast

#include "GEngine/net/net.hpp"
#include "GEngine/cvar/net.hpp"
#include "GEngine/net/msg.hpp"
#include "GEngine/net/net_socket_error.hpp"
#include "GEngine/net/net_wait.hpp"

#include "GEngine/cvar/net.hpp"

#include <algorithm>
#include <cstring>
#include <thread>

#include <iostream>

#ifdef _WIN32
typedef int socklen_t;
typedef u_long ioctlarg_t;

#else
#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#if !defined(__sun) && !defined(__sgi)
#include <ifaddrs.h>
#endif

#endif

namespace Network {

/* Global vars */

NetWait NET::mg_wait;

SocketUDP NET::mg_socketUdp;
SocketTCPMaster NET::mg_socketListenTcp;
SocketUDP NET::mg_socketUdpV6;
SocketTCPMaster NET::mg_socketListenTcpV6;

Event::Manager NET::mg_eventManager;
NetServer NET::mg_server(mg_socketUdp, mg_socketUdpV6);
CLNetClient NET::mg_client(CVar::net_ipv6.getIntValue() ? mg_socketUdpV6 : mg_socketUdp,
                           CVar::net_ipv6.getIntValue() ? AT_IPV6 : AT_IPV4, mg_eventManager.getSocketEvent());

std::vector<IP> NET::g_localIPs;
bool NET::mg_hasListLocalAddress = false;

std::thread NET::mg_networkThread;

std::uint16_t NET::mg_currentUnusedPortTCP = DEFAULT_PORT;
std::uint16_t NET::mg_currentUnusedPortUDP = DEFAULT_PORT;

std::atomic_bool NET::mg_aEnable = false;
std::mutex NET::mg_mutex;

/***************/

bool NET::init(void) {
    if (mg_aEnable)
        return false;

    mg_aEnable = true;
    std::lock_guard<std::mutex> lock(mg_mutex);
    ASocket::initLibs();

    mg_socketUdp = openSocketUdp(mg_currentUnusedPortUDP, false);
    if (CVar::net_ipv6.getIntValue()) // check if ipv6 is supported
        mg_socketUdpV6 = openSocketUdp(mg_currentUnusedPortUDP, true);
    return true;
}

SocketUDP NET::createUDPSocket(const IP &ip) {
    auto sock = openSocketUdp(ip, mg_currentUnusedPortUDP);
    return std::move(sock);
}

bool NET::destroyUDPSocket(SocketUDP &socket) {
    int status = socket.socketClose();
    mg_currentUnusedPortUDP--;
    return status == 0;
}

bool NET::initServer(void) {
    std::lock_guard<std::mutex> lock(mg_mutex);
    if (!NET::mg_aEnable)
        return false;

    mg_socketUdp.setPingResponse();
    if (CVar::net_ipv6.getIntValue())
        mg_socketUdpV6.setPingResponse();
    mg_currentUnusedPortTCP = NET::mg_server.start(mg_currentUnusedPortTCP);
    return true;
}

bool NET::start(void) {
    if (!NET::mg_aEnable)
        return false;

    mg_networkThread = std::thread([]() {
        while (mg_aEnable) {
            try {
                sleep(NET_SLEEP_DURATION);
            } catch (const NetException &e) {
                std::cerr << "FATAL: Network thread exception: " << e.what() << std::endl;

                auto &eventManager = NET::getEventManager();
                eventManager.pushResult(e.getLocation(), !e.shouldRetry());
                if (e.shouldRetry())
                    continue;
                break;
            }
        }
    });

    if (mg_client.isEnabled())
        mg_client.getRecord().startWatchThread();
    return true;
}

bool NET::initClient(void) {
    std::lock_guard<std::mutex> lock(mg_mutex);
    if (!NET::mg_aEnable)
        return false;

    mg_client.init();
    return true;
}

void NET::stop(void) {
    mg_aEnable = false;

    auto &eventManager = NET::getEventManager();
    eventManager.getSocketEvent().signal();
    if (mg_networkThread.joinable())
        mg_networkThread.join();

    /* end of network thread */

    NET::mg_server.stop();
    NET::mg_client.stop();

    g_localIPs.clear();
}

void NET::fetchLocalAddresses(void) {
    mg_hasListLocalAddress = true;

#if defined(__linux__) || defined(__APPLE__) || defined(__BSD__)
    struct ifaddrs *ifap;

    if (getifaddrs(&ifap))
        return; // throw error

    for (ifaddrs *search = ifap; search; search = search->ifa_next)
        if (ifap->ifa_flags & IFF_UP)
            addLocalAddress(search->ifa_name, search->ifa_addr, search->ifa_netmask, search->ifa_flags & IFF_LOOPBACK);

    freeifaddrs(ifap);

#else
    char hostname[256];
    struct addrinfo hint = {0};
    struct addrinfo *res = NULL;

    size_t numIP = 0;

    if (gethostname(hostname, 256) == SOCKET_ERROR)
        return;

    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_DGRAM;

    if (!getaddrinfo(hostname, NULL, &hint, &res)) {
        struct sockaddr_in mask4 = {0};
        struct sockaddr_in6 mask6 = {0};

        mask4.sin_family = AF_INET;
        std::memset(&mask4.sin_addr.s_addr, 0xFF, sizeof(mask4.sin_addr.s_addr));
        mask6.sin6_family = AF_INET6;
        std::memset(&mask6.sin6_addr, 0xFF, sizeof(mask6.sin6_addr));

        // add all IPs from returned list.
        for (struct addrinfo *search = res; search; search = search->ai_next)
            if (search->ai_family == AF_INET)
                addLocalAddress((char *)"", search->ai_addr, (struct sockaddr *)&mask4, search->ai_flags & AI_PASSIVE);
            else if (CVar::net_ipv6.getIntValue() && search->ai_family == AF_INET6)
                addLocalAddress((char *)"", search->ai_addr, (struct sockaddr *)&mask6, search->ai_flags & AI_PASSIVE);
    }

    if (res)
        freeaddrinfo(res);
#endif
}

const std::vector<IP> &NET::getLocalAddresses(void) {
    if (!mg_hasListLocalAddress)
        fetchLocalAddresses();
    return g_localIPs;
}

void NET::addLocalAddress(char *ifname, struct sockaddr *sockaddr, struct sockaddr *netmask, bool isLoopback) {
    // only add addresses that have all required info.
    if (!sockaddr || !netmask || !ifname)
        return;

    sa_family_t family = sockaddr->sa_family;
    size_t addrlen;

    switch (family) {
    case AF_INET:
        addrlen = sizeof(struct sockaddr_in);
        break;
    case AF_INET6:
        addrlen = sizeof(struct sockaddr_in6);
        break;
    default:
        return;
    }

    IP ip = {.family = sockaddr->sa_family};

    if (family == AF_INET) {
        addrlen = sizeof(struct sockaddr_in);
        ip.type = AT_IPV4;
    } else if (family == AF_INET6) {
        addrlen = sizeof(struct sockaddr_in6);
        ip.type = AT_IPV6;
    } else
        return;

    ip.ifname = ifname;

    std::memcpy(&ip.addr, sockaddr, addrlen);
    std::memcpy(&ip.netmask, netmask, addrlen);

    return g_localIPs.push_back(ip);
}

void NET::sortLocalAddresses(void) {
    std::sort(g_localIPs.begin(), g_localIPs.end(), [](const IP &a, const IP &b) {
        if (a.type == AT_LOOPBACK)
            return true;
        if (b.type == AT_LOOPBACK)
            return false;

        if (a.type == AT_IPV4 && b.type == AT_IPV6)
            return true;
        if (a.type == AT_IPV6 && b.type == AT_IPV4)
            return false;

        return false;
    });
}

bool NET::isLanAddress(const Address &addr) {

    if (addr.getType() == AT_LOOPBACK)
        return true;

    return addr.isLanAddr();
}

/**************************************************************/

/* returns true if has event, false otherwise */
bool NET::sleep(uint32_t ms) {
    NetWaitSet set;

    createSets(set);
    bool res = mg_wait.wait(ms, set);
    handleTimeouts();
    if (!res)
        return false;
    return handleEvents(set);
}

void NET::createSets(NetWaitSet &set) {
    set.reset();

    mg_eventManager.createSets(set);
    mg_server.createSets(set);
    mg_client.createSets(set);

    set.setAlert(mg_socketUdp, []() { return NET::handleUdpIPv4(); });
    if (CVar::net_ipv6.getIntValue())
        set.setAlert(mg_socketUdpV6, []() { return NET::handleUdpIPv6(); });
}

bool NET::handleUdpEvent(SocketUDP &socket, UDPMessage &msg, const Address &addr) {
    if (mg_server.handleUDPEvent(socket, msg, addr))
        return true;

    return mg_client.handleUDPEvents(socket, msg, addr);
}

bool NET::handleUdpIPv4(void) {
    UDPMessage msg(0, 0);
    while (true) {
        AddressV4 addr(AT_IPV4, 0);
        bool shouldContinue = mg_socketUdp.receiveV4(msg, addr);
        if (!shouldContinue)
            break;
        handleUdpEvent(mg_socketUdp, msg, addr);
    }
    return true;
}

bool NET::handleUdpIPv6(void) {
    UDPMessage msg(0, 0);
    while (true) {
        AddressV6 addr(AT_IPV6, 0);
        bool shouldContinue = mg_socketUdp.receiveV6(msg, addr);
        if (!shouldContinue)
            break;
        handleUdpEvent(mg_socketUdp, msg, addr);
    }
    return true;
}

bool NET::handleEvents(const NetWaitSet &set) {
#ifdef NET_USE_HANDLE
    set.applyCallback();
    return true;
#else
    if (set.isSignaled(mg_eventManager.getSocketEvent()))
        return mg_eventManager.handleEvent();

    if (set.isSignaled(mg_socketUdp))
        return handleUdpIPv4();
    if (CVar::net_ipv6.getIntValue() && set.isSignaled(mg_socketUdpV6))
        return handleUdpIPv6();

    if (mg_server.handleTCPEvent(set))
        return true;

    return mg_client.handleTCPEvents(set);
#endif
}

void NET::handleTimeouts(void) {
    mg_server.checkTimeouts();
    mg_client.checkTimeouts();
}

/**************************************************************/

/* should it be bool ? should it returns a message instead of sending it directly ? */
void NET::pingServers(void) {
    if (CVar::net_ipv6.getIntValue())
        return mg_client.pingLanServers();
    return mg_client.pingLanServers();
}

} // namespace Network
