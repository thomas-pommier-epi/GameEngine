/*
** EPITECH PROJECT, 2024
** B-CPP-500-LYN-5-1-rtype-basile.fouquet
** File description:
** BaseEngine.inl
*/

#pragma once

namespace gengine {
template <typename T, typename... Params>
inline void BaseEngine::registerSystem(Params &&...p) {
    m_ecs.registerSystem<T>(std::forward<Params>(p)...);
}

template <typename T>
inline void BaseEngine::registerComponent(void) {
    m_ecs.registerComponent<T>();
}

template <typename Type>
void BaseEngine::subscribeCallback(std::function<void(Type &)> callback) {
    m_ecs.subscribeCallback(callback);
}

template <typename T>
void BaseEngine::publishEvent(T &e) {
    m_ecs.publishEvent<T>(e);
}
} // namespace gengine