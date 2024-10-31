/*
** ════════════════════════════════════════════════════════════════════════════
**                           GEngine (libdev) Event
** ════════════════════════════════════════════════════════════════════════════
**  File        : Draw.hpp
**  Create at   : 2024-10-15 05:14
**  Author      : AUTHOR
**  Description : DESCRIPTION // TODO popoche
** ════════════════════════════════════════════════════════════════════════════
*/

#pragma once

#include "GEngine/libdev/Entity.hpp"
#include "GEngine/libdev/System.hpp"
#include "GEngine/libdev/components/driver/output/RaylibTypes.hpp"

namespace gengine::system::event {
class BeginDraw : public Event {
public:
    component::driver::output::Clr clear;
    BeginDraw(component::driver::output::Clr &&clear)
        : clear(clear) {
    }
    BeginDraw(component::driver::output::Clr &clear)
        : clear(clear) {
    }
};
class Draw : public Event {
public:
    Entity entity;
    Draw(Entity &&entity)
        : entity(entity) {
    }
    Draw(Entity entity)
        : entity(entity) {
    }
};
class EndDraw : public Event {};
} // namespace gengine::system::event
