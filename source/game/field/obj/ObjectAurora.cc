#include "ObjectAurora.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief Helper function which contains frequently re-used code. Behavior branches depending on
/// whether it is a full or partial check (call CollisionInfo::updateFloor) or push (push entry in
/// the @ref CollisionDirector).
/// @tparam T The collision info object type, either @ref CollisionInfoPartial or @ref
/// CollisionInfo.
/// @param radius The radius of the collision sphere
/// @param pos The current position of the object
/// @param mask The collision flags
/// @param info Pointer to the collision info structure
/// @param maskOut Pointer to the output collision flags
/// @param timeOffset The time offset for the collision calculation
/// @param push Whether to push a collision entry
/// @return Whether a collision was detected
/// @details If `mask` does not include `KCL_TYPE_FLOOR`, the function will immediately return
/// `false` without performing further collision checks. This check normally occurs after checking
/// the kart's position relative to the wavy road, but we take creative liberty to move this check
/// earlier as a slight performance improvement. If the kart's relative X or Z position are outside
/// the bounds of the wavy road's collision, early returns `false`. Calls @ref calcCollision() with
/// the current race framecount to determine if the kart collided with the wavy road, returning
/// `false` if not. If a collision occurred, updates collision info accordingly based on whether
/// `info` was provided, whether `info` is @ref CollisionInfo or @ref CollisionInfoPartial, whether
/// `maskOut` was provided, and whether `push` is `true`. If the kart's relative Z position is on
/// the last 16% of the wavy road, then treats the collision as #COL_TYPE_BOOST_RAMP, otherwise
/// treats it as trickable #COL_TYPE_ROAD2.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectAurora::checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
        u32 timeOffset, bool push) {
    if (!(mask & KCL_TYPE_FLOOR)) {
        return false;
    }

    EGG::Vector3f relPos = pos - ObjectBase::pos();

    if (relPos.z < 0.0f || relPos.z > COLLISION_SIZE.z ||
            EGG::Mathf::abs(relPos.x) > COLLISION_SIZE.x) {
        return false;
    }

    u32 t = timeOffset + System::RaceManager::Instance()->timer();
    EGG::Vector3f bbox;
    EGG::Vector3f fnrm;
    f32 dist;

    if (!calcCollision(radius, relPos, t, bbox, fnrm, dist)) {
        return false;
    }

    if (info) {
        info->bbox.min = info->bbox.min.minimize(bbox);
        info->bbox.max = info->bbox.max.maximize(bbox);

        if constexpr (std::is_same_v<T, CollisionInfo>) {
            info->updateFloor(dist, fnrm);
        }
    }

    if (maskOut) {
        auto *colDirector = CollisionDirector::Instance();

        if (push) {
            colDirector->pushCollisionEntry(dist, maskOut, KCL_TYPE_BIT(COL_TYPE_ROAD2),
                    COL_TYPE_ROAD2);
            colDirector->setCurrentCollisionVariant(7);
            colDirector->setCurrentCollisionTrickable(true);
        } else {
            *maskOut |= KCL_TYPE_BIT(COL_TYPE_ROAD2);
        }

        if (relPos.z > COLLISION_SIZE.z - 600.0f * 4.0f) {
            if (push) {
                colDirector->pushCollisionEntry(dist, maskOut, KCL_TYPE_BIT(COL_TYPE_BOOST_RAMP),
                        COL_TYPE_BOOST_RAMP);
            } else {
                *maskOut |= KCL_TYPE_BIT(COL_TYPE_ROAD2) | KCL_TYPE_BIT(COL_TYPE_BOOST_RAMP);
            }
        }
    }

    return true;
}

/// @addr{0x807FB060}
/// @brief Calculates the sin-like collision of the wavy road.
/// @param radius The radius of the collision sphere
/// @param relPos The position of the object relative to the wavy road
/// @param time The current framecount
/// @param bbox Out parameter for retrieving the bounding box of the collision (if any)
/// @param fnrm Out parameter for retrieving the floor normal of the collision (if any)
/// @param dist Out parameter for retrieving the depth of the collision (if any)
/// @return Whether a collision was detected
/// @details Computes the Y position of the kart relative to the wavy road's height at time `time`.
/// If the kart is above the wavy road or `600.0f` units below the wavy road, then returns `false`.
/// Otherwise, a collision is occurring. If the kart is more than `300.0f` units below the wavy
/// road, then dampens the collision depth to 20% to avoid excessive upwards snapping of the kart's
/// position. Finally, sets `fnrm` to the world up vector, sets `dist` to the Y-axis collision
/// depth, and sets `bbox` to `dist` in the direction of world up.
bool ObjectAurora::calcCollision(f32 radius, const EGG::Vector3f &relPos, u32 time,
        EGG::Vector3f &bbox, EGG::Vector3f &fnrm, f32 &dist) {
    constexpr f32 COLLISION_DISTANCE_THRESHOLD = 600.0f;
    constexpr f32 UPWARP_THRESHOLD = 300.0f;
    constexpr f32 UPWARP_DIST_SCALAR = 0.2f;

    f32 result = radius - (relPos.y - CalcRoadHeight(relPos.z, time));

    // We're not colliding if we're 600 units below or if the road is below us.
    if (result <= 0.0f || result >= COLLISION_DISTANCE_THRESHOLD) {
        return false;
    }

    // Responsible for the sudden upwards snap that can occur when falling off.
    if (result > UPWARP_THRESHOLD) {
        result *= UPWARP_DIST_SCALAR;
    }

    fnrm = EGG::Vector3f::ey;
    bbox = fnrm * result;
    dist = result;

    return true;
}

// Explicit instantiation, since callers of checkSphereImpl() live in the header and would
// otherwise be unable to see this definition when the class's vtable is emitted.
template bool ObjectAurora::checkSphereImpl<CollisionInfo>(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset, bool push);
template bool ObjectAurora::checkSphereImpl<CollisionInfoPartial>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset, bool push);

} // namespace Kinoko::Field
