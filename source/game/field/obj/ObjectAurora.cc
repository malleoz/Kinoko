#include "ObjectAurora.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief Helper function which contains frequently re-used code. Behavior branches depending on
/// whether it is a push (push entry in the CollisionDirector).
/// @param radius The radius of the collision sphere
/// @param pos The current position of the object
/// @param flags The collision flags
/// @param info Pointer to the collision info structure
/// @param maskOut Pointer to the output collision flags
/// @param timeOffset The time offset for the collision calculation
/// @param push Whether to push a collision entry
/// @return Whether a collision was detected
bool ObjectAurora::checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask flags, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset, bool push) {
    EGG::Vector3f vel = pos - ObjectBase::pos();

    if (vel.z < 0.0f || vel.z > COLLISION_SIZE.z || EGG::Mathf::abs(vel.x) > COLLISION_SIZE.x) {
        return false;
    }

    if (!(flags & KCL_TYPE_FLOOR)) {
        return false;
    }

    u32 t = timeOffset + System::RaceManager::Instance()->timer();
    EGG::Vector3f bbox;
    EGG::Vector3f fnrm;
    f32 dist;

    if (!calcCollision(radius, vel, t, bbox, fnrm, dist)) {
        return false;
    }

    if (info) {
        info->bbox.min = info->bbox.min.minimize(bbox);
        info->bbox.max = info->bbox.max.maximize(bbox);
    }

    if (maskOut) {
        auto *colDirector = CollisionDirector::Instance();

        if (push) {
            colDirector->pushCollisionEntry(dist, maskOut, KCL_TYPE_BIT(COL_TYPE_ROAD2),
                    COL_TYPE_ROAD2);
            colDirector->setCurrentCollisionVariant(7);
            colDirector->setCurrentCollisionTrickable(true);
        } else {
            *maskOut |= KCL_TYPE_BIT(COL_TYPE_ROTATING_ROAD);
        }

        if (vel.z > COLLISION_SIZE.z - 600.0f * 4.0f) {
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

/// @brief Helper function which contains frequently re-used code. Behavior branches depending on
/// whether it is a push (push entry in the CollisionDirector).
/// @param radius The radius of the collision sphere
/// @param pos The current position of the object
/// @param flags The collision flags
/// @param info Pointer to the collision info structure
/// @param maskOut Pointer to the output collision flags
/// @param timeOffset The time offset for the collision calculation
/// @param push Whether to push a collision entry
/// @return Whether a collision was detected
bool ObjectAurora::checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask flags, CollisionInfo *info,
        KCLTypeMask *maskOut, u32 timeOffset, bool push) {
    EGG::Vector3f vel = pos - ObjectBase::pos();

    if (vel.z < 0.0f || vel.z > COLLISION_SIZE.z || EGG::Mathf::abs(vel.x) > COLLISION_SIZE.x) {
        return false;
    }

    if (!(flags & KCL_TYPE_FLOOR)) {
        return false;
    }

    u32 t = timeOffset + System::RaceManager::Instance()->timer();
    EGG::Vector3f bbox;
    EGG::Vector3f fnrm;
    f32 dist;

    if (!calcCollision(radius, vel, t, bbox, fnrm, dist)) {
        return false;
    }

    if (info) {
        info->bbox.min = info->bbox.min.minimize(bbox);
        info->bbox.max = info->bbox.max.maximize(bbox);

        info->updateFloor(dist, fnrm);
    }

    if (maskOut) {
        auto *colDirector = CollisionDirector::Instance();

        if (push) {
            colDirector->pushCollisionEntry(dist, maskOut, KCL_TYPE_BIT(COL_TYPE_ROAD2),
                    COL_TYPE_ROAD2);
            colDirector->setCurrentCollisionVariant(7);
            colDirector->setCurrentCollisionTrickable(true);
        } else {
            *maskOut |= KCL_TYPE_BIT(COL_TYPE_ROTATING_ROAD);
        }

        if (vel.z > COLLISION_SIZE.z - 600.0f * 4.0f) {
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

/// @addr{0x807FAF10}
/// @brief Based off the provided phase and time, calculates the wavy road's surface height for use
/// in collision checks
/// @param phase The phase of the wavy road
/// @param t The current time
/// @return The calculated road height
f32 ObjectAurora::CalcRoadHeight(f32 phase, u32 t) {
    f32 velPeriod = (F_PI * (2.0f * phase)) / COLLISION_SIZE.z;

    f32 result = EGG::Mathf::SinFIdx(
            RAD2FIDX * (velPeriod * TemporalSin(t) + F_PI * static_cast<f32>(t) / 50.0f));

    return velPeriod * 80.0f * result;
}

/// @addr{0x807FAFFC}
/// @brief Computes a sine wave as a function of time
/// @param t The current time
/// @return The calculated sine wave value
f32 ObjectAurora::TemporalSin(f32 t) {
    constexpr f32 PHASE_SHIFT_SECONDS = 30.0f;
    constexpr f32 INITIAL_FREQUENCY = 1.0f;
    constexpr f32 MAX_FREQUENCY = 4.0f;

    f32 minsNormalized = (static_cast<f32>(t) / 60.0f - PHASE_SHIFT_SECONDS) / 60.0f;
    return std::min(INITIAL_FREQUENCY + minsNormalized * minsNormalized, MAX_FREQUENCY);
}

/// @addr{0x807FB060}
/// @brief Calculates the sin-like collision of the wavy road.
/// @param radius The radius of the collision sphere
/// @param vel The current velocity of the object
/// @param time The current framecount
/// @param pos The current position of the object (output)
/// @param fnrm The surface normal at the collision point (output)
/// @param dist The distance to the collision surface (output)
/// @return Whether a collision was detected
/// @details It seems to be modeled as a quadratic chirp with a starting frequency of 1 and a max
/// frequency of 4. The minimum frequency of 1 occurs 30s into the race, and the maximum frequency
/// occurs 2min 13s into the race. The "observed" frequency is dependent on the player's velocity
/// towards the wavy road as well.
bool ObjectAurora::calcCollision(f32 radius, const EGG::Vector3f &vel, u32 time, EGG::Vector3f &pos,
        EGG::Vector3f &fnrm, f32 &dist) {
    constexpr f32 COLLISION_DISTANCE_THRESHOLD = 600.0f;
    constexpr f32 UPWARP_THRESHOLD = 300.0f;
    constexpr f32 UPWARP_DIST_SCALAR = 0.2f;

    f32 result = radius - (vel.y - CalcRoadHeight(vel.z, time));

    // We're not colliding if we're 600 units away or if the road is now behind us.
    if (result <= 0.0f || result >= COLLISION_DISTANCE_THRESHOLD) {
        return false;
    }

    // Responsible for the sudden upwards snap that can occur when falling off.
    if (result > UPWARP_THRESHOLD) {
        result *= UPWARP_DIST_SCALAR;
    }

    fnrm = EGG::Vector3f::ey;
    pos = fnrm * result;
    dist = result;

    return true;
}

} // namespace Kinoko::Field
