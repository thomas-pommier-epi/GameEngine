/*
** ════════════════════════════════════════════════════════════════════════════
**                           GEngine (libdev) System
** ════════════════════════════════════════════════════════════════════════════
**  File        : ClientEventPublisher.hpp
**  Create at   : 2024-10-15 04:49
**  Author      : AUTHOR
**  Description : DESCRIPTION
** ═══════════════════════════════════════════════════════════════════════════
*/

#pragma once

#include "GEngine/interface/events/RemoteLocal.hpp"
#include "GEngine/interface/events/SharedEvent.hpp"
#include "GEngine/libdev/System.hpp"
#include "GEngine/libdev/systems/events/GameLoop.hpp"
#include "GEngine/libdev/systems/events/Native.hpp"
#include "GEngine/net/net.hpp"
#include <iostream>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>

namespace gengine::interface::network::system {

template <class... Events>
class ClientEventPublisher : public System<ClientEventPublisher<Events...>>, public LocalSystem {
public:
    ClientEventPublisher();

    void init(void) override;

    void onStartEngine(gengine::system::event::StartEngine &);

    void onGameLoop(gengine::system::event::GameLoop &);

    void setMe(interface::event::ItsMe &);

private:
    template <typename T>
    void dynamicSubscribe(void);

    static constexpr auto m_maxEventToSend = 20;

    std::uint64_t m_id = 0;
    Network::UDPMessage m_msg;
    std::uint64_t m_eventCount = 0;
    Network::CLNetClient &m_client;
    bool m_ready = false;
    std::unordered_map<std::type_index, std::uint64_t> m_events;

    uuids::uuid m_me;
    mutable std::mutex m_netMutex;
};

#include "ClientEventPublisher.inl" // Inline implementation if needed

} // namespace gengine::interface::network::system
