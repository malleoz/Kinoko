#include "ObjectDrivable.hh"

#include "game/field/ObjectDrivableDirector.hh"

namespace Kinoko::Field {

/// @addr{0x8081A6D0}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectDrivable::ObjectDrivable(const System::MapdataGeoObj &params) : ObjectBase(params) {}

/// @addr{0x8067EB3C}
/// @brief Default virtual destructor
ObjectDrivable::~ObjectDrivable() = default;

/// @brief Creates collision and BoxColUnit, and registers the object to the ObjectDrivableDirector
/// @addr{0x8081A79C}
void ObjectDrivable::load() {
    createCollision();
    initCollision();
    loadAABB(getCollisionRadius());

    ObjectDrivableDirector::Instance()->addObject(this);
}

/// @brief Inserts this object into the BoxColManager as a drivable entry
/// @addr{0x8081A85C}
void ObjectDrivable::loadAABB(f32 radius) {
    auto *boxColMgr = BoxColManager::Instance();
    const EGG::Vector3f &pos = getPosition();
    m_boxColUnit = boxColMgr->insertDrivable(radius, 0.0f, &pos, false, this);
}

} // namespace Kinoko::Field
