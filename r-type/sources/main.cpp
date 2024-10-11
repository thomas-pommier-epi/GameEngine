/*
** EPITECH PROJECT, 2024
** B-CPP-500-LYN-5-1-rtype-basile.fouquet
** File description:
** main.cpp
*/

#include "GEngine/game/Engine.hpp"
#include "GEngine/net/net.hpp"

#include "GEngine/driver/Engine.hpp"
#include "GEngine/game/Engine.hpp"
#include "GEngine/interface/network/Networked.hpp"

#include "GEngine/libdev/components/HitBoxs.hpp"
// #include "libdev/components/Positions.hpp"

#include "GEngine/libdev/systems/Collisions.hpp"
#include "GEngine/libdev/systems/MainLoop.hpp"

#include "GEngine/libdev/components/Velocities.hpp"
#include "GEngine/libdev/systems/Collisions.hpp"
#include "GEngine/libdev/systems/Logger.hpp"
// #include "libdev/components/Positions.hpp"

#include "GEngine/libdev/systems/Collisions.hpp"
#include "GEngine/libdev/systems/Logger.hpp"
#include "GEngine/libdev/systems/MainLoop.hpp"
#include "GEngine/libdev/systems/Motions.hpp"

#include "GEngine/libdev/systems/Motions.hpp"

#include "GEngine/libdev/systems/Collisions.hpp"
#include "GEngine/libdev/systems/Logger.hpp"
#include "GEngine/libdev/systems/MainLoop.hpp"
#include "GEngine/libdev/systems/Motions.hpp"

#include "GEngine/libdev/systems/events/Log.hpp"

#include "GEngine/libdev/systems/driver/input/KeyboardCatcher.hpp"
#include "GEngine/libdev/systems/driver/input/MouseCatcher.hpp"
#include "GEngine/libdev/systems/driver/output/Animate.hpp"
#include "GEngine/libdev/systems/driver/output/Draw.hpp"
#include "GEngine/libdev/systems/driver/output/RenderWindow.hpp"
#include "GEngine/libdev/systems/driver/output/TextureManager.hpp"

#include "GEngine/libdev/components/driver/output/Animation.hpp"
#include "GEngine/libdev/components/driver/output/Shape.hpp"

#include "Start.hpp"

struct TestEvent: public gengine::Event {
    std::string message;
    TestEvent(const std::string &msg): message(msg) {}
};

struct Game: public gengine::OnEventSystem<Game, TestEvent> {
    void onEvent(TestEvent &e) override {
        std::cout << "message: " << e.message << std::endl;
    }
};

struct Driver: public gengine::OnEventSystem<Driver, gengine::system::event::MainLoop> {
    void onEvent(gengine::system::event::MainLoop &e) override {
        publishEvent(TestEvent("hey !"));
    }
};

int main(void) {
    gengine::driver::Engine DriverEngine;
    gengine::game::Engine GameEngine;

    DriverEngine.registerSystem<Driver>();
    GameEngine.registerSystem<Game>();

    gengine::interface::network::Networked interface(DriverEngine, GameEngine, "127.0.0.1", 4243, true);

    interface.run();
    return 0;
}