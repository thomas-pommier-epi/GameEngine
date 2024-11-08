/*
** EPITECH PROJECT, 2024
** GameEngine
** File description:
** RecordManager
*/

#pragma once

#include "GEngine/BaseEngine.hpp"

#include "GEngine/libdev/System.hpp"
#include "GEngine/libdev/systems/events/GameLoop.hpp"
#include "GEngine/libdev/systems/events/Native.hpp"

#include "GEngine/libdev/systems/driver/input/KeyboardCatcher.hpp"

#include "GEngine/interface/network/events/Record.hpp"

namespace gengine::interface::network::system {
class RecordManager : public System<RecordManager>, public LocalSystem {

public:
    void init(void) override;
    void onGameLoop(gengine::system::event::GameLoop &);

    void toggleCapture(gengine::interface::network::event::ToogleRecord &);

private:
    bool m_started = false;
};

} // namespace gengine::interface::network::system
