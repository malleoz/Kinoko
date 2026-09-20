#include "ObjectHighwayManager.hh"

#include "game/field/ObjectDirector.hh"
#include "game/field/obj/ObjectCarTGE.hh"

namespace Kinoko::Field {

/// @addr{0x806D2908}
/// @brief Constructor
/// @pre All @ref ObjectCarTGE objects must be constructed and registered to the vector of managed
/// objects in @ref ObjectDirector. Otherwise, their squash cooldowns will act independently of
/// other cars.
/// @details Counts the number of managed cars and trucks and sizes @ref m_cars accordingly. For
/// each of these managed cars/trucks, caches a pointer to that object and sets its highway manager
/// to this instance.
ObjectHighwayManager::ObjectHighwayManager()
    : ObjectCollidable("HighwayManager", EGG::Vector3f::zero, EGG::Vector3f::ez,
              EGG::Vector3f::unit) {
    size_t carCount = 0;
    auto *objDir = ObjectDirector::Instance();

    for (auto *const &obj : objDir->managedObjects()) {
        const char *objName = obj->getName();

        if ((strcmp(objName, "car_body") == 0) || (strcmp(objName, "kart_truck") == 0)) {
            ++carCount;
        }
    }

    m_cars = owning_span<ObjectCarTGE *>(carCount);
    size_t idx = 0;

    for (auto *&obj : objDir->managedObjects()) {
        const char *objName = obj->getName();

        if ((strcmp(objName, "car_body") == 0) || (strcmp(objName, "kart_truck") == 0)) {
            auto *carObj = reinterpret_cast<ObjectCarTGE *>(obj);
            m_cars[idx++] = carObj;
            carObj->setHighwayManager(this);
        }
    }
}

/// @addr{0x806D50AC}
/// @brief Iterates all vehicles and resets the squash timer if any vehicle has squashed the player
/// @details If any vehicle has squashed the player and the player is currently vulnerable, resets
/// the squash timer to 0. Otherwise, increments the squash timer up to the maximum value defined by
/// @ref SQUASH_MAX.
void ObjectHighwayManager::calcSquash() {
    constexpr u32 SQUASH_INVULNERABILITY = 200;

    bool vulnerable = m_squashTimer >= SQUASH_INVULNERABILITY;

    for (auto *&obj : m_cars) {
        if (obj->squashed() && vulnerable) {
            m_squashTimer = 0;
        }

        obj->reset();
    }

    m_squashTimer = std::min<u32>(m_squashTimer + 1, SQUASH_MAX);
}

} // namespace Kinoko::Field
