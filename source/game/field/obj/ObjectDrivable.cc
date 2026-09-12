#include "ObjectDrivable.hh"

#include "game/field/ObjectDrivableDirector.hh"

namespace Kinoko::Field {

/// @addr{0x8081A79C}
/// @copybrief ObjectBase::load()
/// @details Creates and initializes the object's collision and associated @ref BoxColUnit, and
/// registers the object to the @ref ObjectDrivableDirector.
void ObjectDrivable::load() {
    createCollision();
    initCollision();
    loadAABB(getCollisionRadius());

    ObjectDrivableDirector::Instance()->addObject(this);
}

} // namespace Kinoko::Field
