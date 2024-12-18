/*
** EPITECH PROJECT, 2024
** GameEngine
** File description:
** RemoteLocal.hpp
*/

#pragma once

#include "GEngine/libdev/Component.hpp"
#include <sstream>
#include <stduuid/uuid.h>
#include <string>

namespace gengine::interface::component {

class RemoteLocal : public gengine::Component<RemoteLocal> {
public:
    // Constructor - Generates a new UUID upon object creation
    RemoteLocal();

    RemoteLocal(const uuids::uuid &uuid);

    // Copy constructor
    RemoteLocal(const RemoteLocal &other);

    // Assignment operator
    RemoteLocal &operator=(const RemoteLocal &other);

    // Overloading the == operator to compare based on UUID
    bool operator==(const RemoteLocal &other) const;

    // Getter for the UUID as a string (hexadecimal format)
    std::string getUUIDString() const;

    // Getter for the raw UUID bytes (for network transmission)
    const uuids::uuid &getUUIDBytes() const;

    bool operator<(const RemoteLocal &other) const {
        return false;
    }

    static void generateUUID(uuids::uuid &toGenerate);

    void setWhoIAm(const uuids::uuid &toSet) {
        m_whoIAm = toSet;
    }

    uuids::uuid getWhoIAm(void) const {
        return m_whoIAm;
    }

private:
    uuids::uuid m_uuid;
    uuids::uuid m_whoIAm;
};
} // namespace gengine::interface::component

namespace std {
template <>
struct hash<gengine::interface::component::RemoteLocal> {
    std::size_t operator()(const gengine::interface::component::RemoteLocal &driver) const {
        const uuids::uuid &uuid = driver.getUUIDBytes();
        std::size_t hash_value = 0;
        auto bytes = uuid.as_bytes();
        for (const auto byte : bytes)
            hash_value ^= std::hash<uint8_t>{}((uint8_t)byte) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
        return hash_value;
    }
};
} // namespace std
