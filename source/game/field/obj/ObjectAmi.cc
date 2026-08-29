#include "ObjectAmi.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x80807ED0}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectAmi::ObjectAmi(const System::MapdataGeoObj &params) : ObjectDrivable(params) {}

/// @addr{0x80808860}
/// @brief Default virtual destructor
ObjectAmi::~ObjectAmi() = default;

/// @brief Helper function which contains frequently re-used code. Behavior branches depending on
/// whether it is a full or partial check (call CollisionInfo::updateFloor) or push (push entry in
/// the CollisionDirector).
/// @tparam T The CollisionInfo object type, either CollisionInfoPartial or CollisionInfo.
/// @param push Whether to push a collision entry
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectAmi::checkSphereImpl(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f & /*v1*/,
        KCLTypeMask flags, T *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset, bool push) {
    // We check flags first to avoid unnecessary position posDelta computation.
    if (!(flags & KCL_TYPE_FLOOR)) {
        return false;
    }

    EGG::Vector3f posDelta = v0 - pos();
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

    if (pInfo) {
        pInfo->bbox.min = pInfo->bbox.min.minimize(bbox);
        pInfo->bbox.max = pInfo->bbox.max.maximize(bbox);

        if constexpr (std::is_same_v<T, CollisionInfo>) {
            pInfo->updateFloor(dist, fnrm);
        }
    }

    if (pFlagsOut) {
        if (push) {
            auto *colDirector = CollisionDirector::Instance();
            colDirector->pushCollisionEntry(dist, pFlagsOut, KCL_TYPE_BIT(COL_TYPE_ROTATING_ROAD),
                    COL_TYPE_ROTATING_ROAD);
            colDirector->setCurrentCollisionVariant(0);
            colDirector->setCurrentCollisionTrickable(true);
        } else {
            *pFlagsOut |= KCL_TYPE_BIT(COL_TYPE_ROTATING_ROAD);
        }

        if (posDelta.z > DIMS.z) {
            *pFlagsOut |= KCL_TYPE_BIT(COL_TYPE_BOOST_RAMP);
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
