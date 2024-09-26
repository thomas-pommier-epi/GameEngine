/*
** EPITECH PROJECT, 2024
** B-CPP-500-LYN-5-1-rtype-basile.fouquet
** File description:
** Start.hpp
*/

#pragma once

#include "libdev/System.hpp"
#include "libdev/systems/events/Native.hpp"

namespace hagarioop::systems {
    class Start: public gengine::System<Start> {
    public:
        void init(void) override;
        void onStartEngine(gengine::system::event::StartEngine &);
    };
}
