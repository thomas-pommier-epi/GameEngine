/*
** ════════════════════════════════════════════════════════════════════════════
**                           GEngine (libdev) System
** ════════════════════════════════════════════════════════════════════════════
**  File        : RenderWindow.hpp
**  Create at   : 2024-10-15 05:01
**  Author      : AUTHOR
**  Description : DESCRIPTION // TODO popoche
** ═══════════════════════════════════════════════════════════════════════════
*/

#pragma once

#include "module/raylib_safe.h"

#include "GEngine/libdev/System.hpp"
#include "GEngine/libdev/systems/events/MainLoop.hpp"
#include "GEngine/libdev/systems/events/Native.hpp"
#include "GEngine/libdev/systems/events/RenderLoop.hpp"
#include "GEngine/libdev/systems/events/driver/output/Draw.hpp"
#include "GEngine/libdev/systems/events/driver/output/Window.hpp"

namespace gengine::system::driver::output {
class RenderWindow : public gengine::System<RenderWindow>, public LocalSystem {
public:
    RenderWindow(int width, int height, const std::string &title);

    void init(void) override;

    void onStartEngine(gengine::system::event::StartEngine &e);

    void onMainLoop(gengine::system::event::MainLoop &e);

    int getWidth(void) const;
    void setWidth(int width);
    int getHeight(void) const;
    void setHeight(int height);
    const std::string &getTitle(void) const;
    void setTitle(const std::string &title);

private:
    int m_baseWidth;
    int m_baseHeight;
    float m_width;
    float m_height;
    int m_trueWidth;
    int m_trueHeight;
    std::string m_title;
};
} // namespace gengine::system::driver::output
