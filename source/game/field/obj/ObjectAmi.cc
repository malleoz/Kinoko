#include "ObjectAmi.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x808088A0}
/// @brief Private function that checks collision between a sphere and the object, writing
/// partial collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool ObjectAmi::checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, false);
}

/// @addr{0x80808A8C}
/// @brief Private function that checks collision between a sphere and the object, writing
/// partial collision info. Additionally pushes the collision entry into the @ref
/// CollisionDirector cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool ObjectAmi::checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, true);
}

/// @addr{0x80808CA8}
/// @brief Private function that checks collision between a sphere and the object, writing
/// out full collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool ObjectAmi::checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, false);
}

/// @addr{0x80809060}
/// @brief Private function that checks collision between a sphere and the object, writing
/// out full collision info. Additionally pushes the collision entry into the @ref
/// CollisionDirector cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool ObjectAmi::checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, true);
}

/// @brief Helper function which contains frequently re-used code. Behavior branches depending on
/// whether it is a full or partial check (call CollisionInfo::updateFloor) or push (push entry in
/// the CollisionDirector).
/// @tparam T The collision info object type, either @ref CollisionInfoPartial or @ref
/// CollisionInfo.
/// @param radius The radius of the sphere to check
/// @param pos The current position of the sphere to check
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @param push Whether to push a collision entry
/// @return Whether a collision was detected
/// @details If `mask` does not include `KCL_TYPE_FLOOR`, the function will immediately return
/// `false` without performing further collision checks. This check normally occurs after checking
/// the kart's position relative to the net, but we take creative liberty to move this check earlier
/// as a slight performance improvement. If the kart's Z position is outside the net's bounds, early
/// returns `false` without performing a collision check. Calls @ref checkCollision() using the
/// computed relative position. If a collision did not occur, returns `false`. Otherwise, updates
/// collision information accordingly, depending on whether `info` was provided, the type of
/// collision info object, whether `maskOut` was provided, and whethera `push` is requested.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectAmi::checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
        u32 timeOffset, bool push) {
    // We check flags first to avoid unnecessary position posDelta computation.
    if (!(mask & KCL_TYPE_FLOOR)) {
        return false;
    }

    EGG::Vector3f posDelta = pos - ObjectBase::pos();
    posDelta.z *= -1.0f;

    if (posDelta.z < 0.0f || posDelta.z > DIMS.z || EGG::Mathf::abs(posDelta.x) > DIMS.x) {
        return false;
    }

    u32 t = timeOffset + System::RaceManager::Instance()->timer();
    EGG::Vector3f bbox;
    EGG::Vector3f fnrm;
    f32 dist;

    if (!checkCollision(radius, posDelta, t, bbox, fnrm, dist)) {
        return false;
    }

    if (info) {
        info->updateBBox(bbox);

        if constexpr (std::is_same_v<T, CollisionInfo>) {
            info->updateFloor(dist, fnrm);
        }
    }

    if (maskOut) {
        if (push) {
            auto *colDirector = CollisionDirector::Instance();
            colDirector->pushCollisionEntry(dist, maskOut, KCL_TYPE_BIT(COL_TYPE_ROTATING_ROAD),
                    COL_TYPE_ROTATING_ROAD);
            colDirector->setCurrentCollisionVariant(0);
            colDirector->setCurrentCollisionTrickable(true);
        } else {
            *maskOut |= KCL_TYPE_BIT(COL_TYPE_ROTATING_ROAD);
        }

        if (posDelta.z > DIMS.z) {
            *maskOut |= KCL_TYPE_BIT(COL_TYPE_BOOST_RAMP);
        }
    }

    return true;
}

/// @addr{0x80808308}
/// @brief Calculates the hit depth of the net and writes out collision information if a collision
/// is detected
/// @param radius The radius of the hitbox to check against the net
/// @param posDelta The position of the hitbox relative to the net's position
/// @param time The current race time
/// @param bbox Out parameter for retrieving the bounding box of the collision (if any)
/// @param fnrm Out parameter for retrieving the floor normal of the collision (if any)
/// @param dist Out parameter for retrieving the depth of the collision (if any)
/// @return `true` if a collision with the net was detected, `false` otherwise.
/// @details If the kart's relative position along the Y-axis is above the net's surface or is
/// `600.0f` units below the net, no collision is detected. Otherwise, the kart is considered to be
/// colliding with the net. If the kart is more than `300.0f` units below the net, the collision
/// depth is reduced to 20% of its original value to prevent excessive position snapping for the
/// kart. Sets `fnrm` to the floor normal of the net's surface, `dist` based on the collision depth,
/// and `bbox` based on `fnrm * dist`.
bool ObjectAmi::checkCollision(f32 radius, const EGG::Vector3f &posDelta, u32 time,
        EGG::Vector3f &bbox, EGG::Vector3f &fnrm, f32 &dist) {
    constexpr EGG::Vector3f FLOOR_NORMAL = EGG::Vector3f(0.0f, 1.5f, -0.5f);

    f32 depth = radius - (posDelta.y - calcNetHeight(posDelta.z, time));

    // Not colliding if net is falling below the hitbox's radius or above by more than 600 units.
    if (depth <= 0.0f || depth >= 600.0f) {
        return false;
    }

    if (depth >= 300.0f) {
        depth *= 0.2f;
    }

    fnrm = FLOOR_NORMAL;
    fnrm.normalise();
    bbox = fnrm * depth;
    dist = depth;

    return true;
}

} // namespace Kinoko::Field
