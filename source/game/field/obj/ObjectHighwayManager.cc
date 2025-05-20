#include "ObjectHighwayManager.hh"

#include "game/field/obj/ObjectCarTGE.hh"

#include "game/field/ObjectDirector.hh"

namespace Field {

/// @addr{0x806D2908}
ObjectHighwayManager::ObjectHighwayManager()
    : ObjectCollidable("HighwayManager", EGG::Vector3f::zero, EGG::Vector3f::ez,
              EGG::Vector3f(1.0f, 1.0f, 1.0f)) {
    m_carCount = 0;
    m_truckCount = 0;

    auto *objDir = ObjectDirector::Instance();
    size_t managedCount = objDir->managedCount();
    for (size_t i = 0; i < managedCount; ++i) {
        auto *obj = objDir->managedObjectByIdx(i);
        const char *objName = obj->getName();

        if (strcmp(objName, "car_body") == 0) {
            ++m_carCount;
        } else if (strcmp(objName, "kart_truck") == 0) {
            ++m_truckCount;
        }
    }

    m_cars = std::span<ObjectCarTGE *>(new ObjectCarTGE *[m_carCount], m_carCount);
    m_trucks = std::span<ObjectCarTGE *>(new ObjectCarTGE *[m_truckCount], m_truckCount);
    u32 carIdx = 0;
    u32 truckIdx = 0;

    for (size_t i = 0; i < managedCount; ++i) {
        auto *obj = objDir->managedObjectByIdx(i);
        const char *objName = obj->getName();

        if (strcmp(objName, "car_body") == 0) {
            m_cars[carIdx++] = reinterpret_cast<ObjectCarTGE *>(obj);
        } else if (strcmp(objName, "kart_truck") == 0) {
            m_trucks[truckIdx++] = reinterpret_cast<ObjectCarTGE *>(obj);
        }
    }

    u32 allCount = m_carCount + m_truckCount;
    m_all = std::span<ObjectCarTGE *>(new ObjectCarTGE *[allCount], allCount);
    std::copy(m_cars.begin(), m_cars.end(), m_all.begin());
    std::copy(m_trucks.begin(), m_trucks.end(), m_all.begin() + m_carCount);

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

/// @addr{0x806D345C}
void ObjectHighwayManager::calc() {
    calcSquash();
}

/// @addr{0x806D50AC}
void ObjectHighwayManager::calcSquash() {
    constexpr u32 SQUASH_INVULNERABILITY = 200;
    constexpr u32 SQUASH_MAX = 600;

    bool vulnerable = m_squashTimer >= SQUASH_INVULNERABILITY;

    for (auto *obj : m_all) {
        if (obj->squashed() && vulnerable) {
            m_squashTimer = 0;
        }

        obj->reset();
    }

    m_squashTimer = std::min<u32>(m_squashTimer + 1, SQUASH_MAX);
}

} // namespace Field
