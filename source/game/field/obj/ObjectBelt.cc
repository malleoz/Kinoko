#include "ObjectBelt.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x807FC294}
/// @brief Checks if an object at a given position is colliding with the conveyer and if so, saves
/// the road velocity to the @ref CollisionInfo
/// @param pos The position of the object being checked for collision
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset The time offset to use when calculating the road velocity
/// @return Whether or not a collision with the conveyer belt was detected
bool ObjectBelt::calcCollision(const EGG::Vector3f &pos, const EGG::Vector3f & /*prevPos*/,
        KCLTypeMask /*mask*/, CollisionInfo *info, KCLTypeMask *maskOut, u32 timeOffset) {
    if ((*maskOut & KCL_TYPE_BIT(COL_TYPE_MOVING_ROAD)) == 0) {
        return false;
    }

    auto *colDir = CollisionDirector::Instance();
    if (!colDir->findClosestCollisionEntry(maskOut, KCL_TYPE_BIT(COL_TYPE_MOVING_ROAD))) {
        return false;
    }

    const auto *entry = colDir->closestCollisionEntry();
    if (!isMoving(entry->variant(), pos)) {
        return false;
    }

    if (entry->dist > info->movingFloorDist) {
        info->movingFloorDist = entry->dist;
        info->roadVelocity = calcRoadVelocity(entry->variant(), pos,
                System::RaceManager::Instance()->timer() - timeOffset);
    }

    return true;
}

} // namespace Kinoko::Field
