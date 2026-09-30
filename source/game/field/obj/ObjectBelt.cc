#include "ObjectBelt.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x807FC294}
/// @brief Checks if an object at a given position is colliding with the conveyer and if so, saves
/// the road velocity to the @ref CollisionInfo
/// @param pos The position of the object being checked for collision
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the course KCL check and this collision check
/// (if any)
/// @param timeOffset The time offset to use when calculating the road velocity
/// @return Whether or not a collision with the conveyer belt was detected
/// @details Early returns `false` if `maskOut` does not have #COL_TYPE_MOVING_ROAD set. Next,
/// ensures that the closest collision entry in the @ref CollisionDirector is of type
/// #COL_TYPE_MOVING_ROAD, returning `false` if this is not the case. Next, checks to see if the
/// conveyer belt is actually moving, returning `false` if it is stationary. Finally, if a collision
/// is detected, sets the collision info's moving floor distance and road velocity based on the
/// player's position, the conveyer belt's variant, and the race framecount.
/// @note This function is unique from other collision check functions in that it requires that
/// `maskOut` has already had #COL_TYPE_MOVING_ROAD set before calling. Conveyer belts will only
/// register collisions if the conveyer belt is placed on #COL_TYPE_MOVING_ROAD present in the
/// course KCL.
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
