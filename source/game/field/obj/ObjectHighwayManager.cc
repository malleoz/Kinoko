#include "ObjectHighwayManager.hh"

#include "game/field/obj/ObjectCarTGE.hh"
#include "game/field/obj/ObjectKStick.hh"

#include "game/field/ObjectDirector.hh"

namespace Field {

/// @addr{0x806D2908}
ObjectHighwayManager::ObjectHighwayManager()
    : ObjectCollidable("HighwayManager", EGG::Vector3f::zero, EGG::Vector3f::ez,
              EGG::Vector3f(1.0f, 1.0f, 1.0f)) {
    size_t carCount = 0;
    size_t truckCount = 0;
    size_t stickCount = 0;

    auto *objDir = ObjectDirector::Instance();
    size_t managedCount = objDir->managedCount();
    for (size_t i = 0; i < managedCount; ++i) {
        auto *obj = objDir->managedObjectByIdx(i);
        const char *objName = obj->getName();

        if (strcmp(objName, "car_body") == 0) {
            ++carCount;
        } else if (strcmp(objName, "kart_truck") == 0) {
            ++truckCount;
        } else if (strcmp(objName, "Ksticketc") == 0) {
            ++stickCount;
        }
    }

    m_cars = std::span<ObjectCarTGE *>(new ObjectCarTGE *[carCount], carCount);
    m_trucks = std::span<ObjectCarTGE *>(new ObjectCarTGE *[truckCount], truckCount);
    m_sticks = std::span<ObjectKStick *>(new ObjectKStick *[stickCount], stickCount);
    u32 carIdx = 0;
    u32 truckIdx = 0;
    u32 stickIdx = 0;

    for (size_t i = 0; i < managedCount; ++i) {
        auto *obj = objDir->managedObjectByIdx(i);
        const char *objName = obj->getName();

        if (strcmp(objName, "car_body") == 0) {
            m_cars[carIdx++] = reinterpret_cast<ObjectCarTGE *>(obj);
        } else if (strcmp(objName, "kart_truck") == 0) {
            m_trucks[truckIdx++] = reinterpret_cast<ObjectCarTGE *>(obj);
        } else if (strcmp(objName, "Ksticketc") == 0) {
            m_sticks[stickIdx++] = reinterpret_cast<ObjectKStick *>(obj);
        }
    }

    u32 allCount = m_cars.size() + m_trucks.size();
    m_all = std::span<ObjectCarTGE *>(new ObjectCarTGE *[allCount], allCount);
    std::copy(m_cars.begin(), m_cars.end(), m_all.begin());
    std::copy(m_trucks.begin(), m_trucks.end(), m_all.begin() + m_cars.size());

    for (auto *&obj : m_all) {
        obj->setHighwayManager(this);
    }
}

/// @addr{0x806D2FE8}
ObjectHighwayManager::~ObjectHighwayManager() {
    // The manager does not own the underlying objects. Only free the array itself.
    delete[] m_cars.data();
    delete[] m_trucks.data();
    delete[] m_all.data();
}

/// @addr{0x806D332C}
void ObjectHighwayManager::init() {
    m_squashTimer = SQUASH_MAX;
}

/// @addr{0x806D345C}
void ObjectHighwayManager::calc() {
    calcSquash();
    calcSticks();
}

/// @addr{0x806D50AC}
void ObjectHighwayManager::calcSquash() {
    constexpr u32 SQUASH_INVULNERABILITY = 200;

    bool vulnerable = m_squashTimer >= SQUASH_INVULNERABILITY;

    for (auto *obj : m_all) {
        if (obj->squashed() && vulnerable) {
            m_squashTimer = 0;
        }

        obj->reset();
    }

    m_squashTimer = std::min<u32>(m_squashTimer + 1, SQUASH_MAX);
}

/// @addr{0x806D51C8}
/// @brief Checks car collision against toll booth arms. We don't really care about them, rather we
/// need this function to sync since it performs a collision check update.
void ObjectHighwayManager::calcSticks() {
    for (auto *&obj : m_all) {
        if (!obj->collidable()) {
            continue;
        }

        obj->calcCollisionTransform();

        EGG::Vector3f dummy;
        for (auto *&stick : m_sticks) {
            obj->checkCollision(stick->collision(), dummy);
        }
    }
}

} // namespace Field
