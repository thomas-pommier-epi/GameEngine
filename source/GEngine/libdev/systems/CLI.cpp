/*
** EPITECH PROJECT, 2024
** GameEngine
** File description:
** CLI.cpp
*/

#include "GEngine/libdev/systems/CLI.hpp"

#include "GEngine/net/events/socket_event.hpp"
#include "GEngine/net/net_socket_system.hpp"
#include "GEngine/net/net_wait.hpp"

namespace gengine::system {

CLI::CLI()
    : m_stopReading(false) {
}

void CLI::init(void) {
    subscribeToEvent<gengine::system::event::StartEngine>(&CLI::onStartEngine);
    subscribeToEvent<gengine::system::event::StopEngine>(&CLI::onStopEngine);
    subscribeToEvent<gengine::system::event::MainLoop>(&CLI::onMainLoop);
}

void CLI::onStartEngine(gengine::system::event::StartEngine &e [[maybe_unused]]) {
    m_socketSTD.setStd(STD_IN);
#ifndef NET_USE_HANDLE
    m_wait.addSocketPool(m_socketSTD);
    m_wait.addSocketPool(m_socketEventStop);
#endif
    m_inputThread = std::thread(&CLI::getInputs, this);
}

void CLI::onStopEngine(gengine::system::event::StopEngine &e [[maybe_unused]]) {
    m_stopReading = true;

    m_socketEventStop.signal();

#ifdef NET_USE_HANDLE
    SetConsoleMode(m_socketSTD.getHandle(), m_dwMod);
#endif

    if (m_inputThread.joinable())
        m_inputThread.join();
}

void CLI::onMainLoop(gengine::system::event::MainLoop &e [[maybe_unused]]) {
    std::lock_guard<std::mutex> lock(m_historyMutex);
    if (m_stopProgram) {
        publishEvent(gengine::system::event::StopMainLoop());
        return;
    }

    for (const auto &entry : m_userInputHistory)
        publishEvent(gengine::system::event::CLINewInput(splitInput(entry)));
    m_userInputHistory.clear();
}

void CLI::processOutput(void) {
    std::string input;

    auto &res = std::getline(std::cin, input);
    if (res.eof()) {
        m_stopProgram = true;
        m_stopReading = true;
        return;
    }

    if (!input.empty()) {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_userInputHistory.push_back(input);
    }

    /* ugly way of output a > once it's printed */
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void CLI::getInputs(void) {
#ifdef NET_USE_HANDLE
    GetConsoleMode(m_socketSTD.getHandle(), &m_dwMod);

    DWORD fdwMode = m_dwMod & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
    SetConsoleMode(m_socketSTD.getHandle(), fdwMode);

    /* flush to remove existing events */
    FlushConsoleInputBuffer(m_socketSTD.getHandle());
#endif
    while (!m_stopReading) {
        Network::NetWaitSet set;

        set.setAlert(m_socketEventStop, [this]() {
            /* do nothing, just loopback since the thread must be killed */
            return true;
        });

        set.setAlert(m_socketSTD, [this]() {
            processOutput();
            return true;
        });

        std::cout << "> " << std::flush;
        bool hasActivity = m_wait.wait(1000000000, set);
        if (!hasActivity)
            continue;

#ifdef NET_USE_HANDLE
        set.applyCallback(false);
#else
        if (set.isSignaled(m_socketEventStop)) /**/
            break;
        if (set.isSignaled(m_socketSTD))
            processOutput();
#endif
    }
}

std::vector<std::string> CLI::splitInput(const std::string &input) {
    std::vector<std::string> words;
    std::istringstream stream(input);
    std::string word;

    while (stream >> word)
        words.push_back(word);
    return words;
}
} // namespace gengine::system
