#include "ObjectHighwayManager.hh"

#include "game/field/obj/ObjectCarTGE.hh"

#include "game/field/ObjectDirector.hh"

namespace Field {

/// @addr{0x806D2908}
ObjectHighwayManager::ObjectHighwayManager()
    : ObjectCollidable("HighwayManager", EGG::Vector3f::zero, EGG::Vector3f::ez,
              EGG::Vector3f(EGG::Vector3f::unit)) {
    size_t carCount = 0;
    auto *objDir = ObjectDirector::Instance();

    for (auto *&obj : objDir->managedObjects()) {
        const char *objName = obj->getName();

        if (strcmp(objName, "car_body") == 0) {
            ++carCount;
        } else if (strcmp(objName, "kart_truck") == 0) {
            ++carCount;
        }
    }

    m_cars = std::span<ObjectCarTGE *>(new ObjectCarTGE *[carCount], carCount);
    u32 idx = 0;

    for (auto *&obj : objDir->managedObjects()) {
        const char *objName = obj->getName();

        if (strcmp(objName, "car_body") == 0) {
            m_cars[idx++] = reinterpret_cast<ObjectCarTGE *>(obj);
        } else if (strcmp(objName, "kart_truck") == 0) {
            m_cars[idx++] = reinterpret_cast<ObjectCarTGE *>(obj);
        }
    }

    for (auto *&obj : m_cars) {
        obj->setHighwayManager(this);
    }
}

/// @addr{0x806D2FE8}
ObjectHighwayManager::~ObjectHighwayManager() {
    // The manager does not own the underlying objects. Only free the array itself.
    delete[] m_cars.data();
}

/// @addr{0x806D332C}
void ObjectHighwayManager::init() {
    m_squashTimer = SQUASH_MAX;
}

/// @addr{0x806D345C}
void ObjectHighwayManager::calc() {
    calcSquash();
}

/// @addr{0x806D50AC}
void ObjectHighwayManager::calcSquash() {
    constexpr u32 SQUASH_INVULNERABILITY = 200;

    bool vulnerable = m_squashTimer >= SQUASH_INVULNERABILITY;

    for (auto *obj : m_cars) {
        if (obj->squashed() && vulnerable) {
            m_squashTimer = 0;
        }

        obj->reset();
    }

    m_squashTimer = std::min<u32>(m_squashTimer + 1, SQUASH_MAX);
}

} // namespace Field
