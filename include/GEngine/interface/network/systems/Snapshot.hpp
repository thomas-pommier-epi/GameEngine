/*
** ════════════════════════════════════════════════════════════════════════════
**                           GEngine (libdev) System
** ════════════════════════════════════════════════════════════════════════════
**  File        : Snapshot.hpp
**  Create at   : 2024-10-15 04:51
**  Author      : AUTHOR
**  Description : DESCRIPTION
** ═══════════════════════════════════════════════════════════════════════════
*/

#pragma once

#include <array>
#include <mutex>

#include "GEngine/BaseEngine.hpp"

#include "GEngine/libdev/System.hpp"
#include "GEngine/libdev/systems/events/GameLoop.hpp"
#include "GEngine/libdev/systems/events/Native.hpp"

#include "GEngine/interface/components/RemoteLocal.hpp"
#include "GEngine/interface/events/RemoteLocal.hpp"
#include "GEngine/interface/network/components/NetSend.hpp"
#include "GEngine/interface/network/systems/ServerClients.hpp"

#define MAX_SNAPSHOT 60

namespace gengine::interface::network::system {

class Snapshot : public System<Snapshot, component::NetSend, interface::component::RemoteLocal,
                               gengine::interface::network::system::ServerClientsHandler>,
                 public RemoteSystem {
public:
    using snapshot_t = BaseEngine::world_t;
    using snapshots_t = std::array<snapshot_t, MAX_SNAPSHOT>;

    Snapshot(const snapshot_t &currentWorld);

    void init(void) override;
    void onGameLoop(gengine::system::event::GameLoop &);

    void registerSnapshot(gengine::interface::event::NewRemoteLocal &e);
    void destroySnapshot(gengine::interface::event::DeleteRemoteLocal &e);
    void getAndSendDeltaDiff(void);

private:
    const snapshot_t &m_currentWorld;
    snapshot_t m_dummySnapshot;
    std::unordered_map<uuids::uuid, std::pair<uint64_t, snapshots_t>> m_clientSnapshots;
    uint64_t m_currentSnapshotId = -1;

    std::pair<std::vector<uint8_t>, std::map<ecs::component::ComponentTools::component_id_t, const std::any>>
    getDeltaDiff(ecs::entity::Entity entity, const snapshot_t &snap1, const snapshot_t &snap2) const;
};
} // namespace gengine::interface::network::system