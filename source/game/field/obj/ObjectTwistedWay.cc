#include "ObjectTwistedWay.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Field {

/// @brief Helper function which contains frequently re-used code. Behavior branches depending on
/// whether it is a full or partial check (call CollisionInfo::updateFloor) or push (push entry in
/// the @ref CollisionDirector).
/// @tparam T The type of collision information structure (@ref CollisionInfo or @ref
/// CollisionInfoPartial)
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @param push Whether to push the collision entry to the @ref CollisionDirector
/// @return Whether a collision was detected
/// @details If the kart's Z-position is more than @ref HALF_DEPTH units away from the wavy road's
/// position, or if the kart's X-position is more than twice the @ref HALF_WIDTH units away, this
/// function early returns `false` and does not perform a collision check. Establishes the current
/// framecount to use, which is the race manager's timer if in a race, or the intro timer if the
/// race hasn't started, plus the optional `timeOffset`. This framecount is then doubled to derive
/// the time parameter `t` used in the wave angle calculation. Calculates the angle of the wavy road
/// at the current position and time. Finally, depending on which type of collision check is being
/// performed (wall and/or floor), the appropriate collision check function is called with the
/// calculated angle.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectTwistedWay::checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
        u32 timeOffset, bool push) {
    EGG::Vector3f relPos = pos - ObjectTwistedWay::pos();

    if (EGG::Mathf::abs(relPos.z) > HALF_DEPTH || EGG::Mathf::abs(relPos.x) > HALF_WIDTH * 2.0f) {
        return false;
    }

    auto *raceMgr = System::RaceManager::Instance();
    bool isInRace = raceMgr->isStageReached(System::RaceManager::Stage::Race);

    u32 frameCount = timeOffset + (isInRace ? raceMgr->timer() : m_introTimer);
    u32 t = (frameCount % PERIOD_LENGTH) * 2;

    f32 angle = isInRace ? CalcWaveAngle(-relPos.z / HALF_DEPTH, t) : 0.0f;

    bool hasCol = false;
    if (mask & KCL_TYPE_BIT(COL_TYPE_WALL)) {
        hasCol |= checkWallCollision(angle, radius, t, relPos, info, maskOut, push);
    }

    if (mask & KCL_TYPE_BIT(COL_TYPE_ROAD)) {
        hasCol |= checkFloorCollision(angle, radius, relPos, info, maskOut, push);
    }

    return hasCol;
}

/// @brief Helper function which contains frequently re-used code for checking wall collisions
/// @tparam T The type of collision information structure (@ref CollisionInfo or @ref
/// CollisionInfoPartial)
/// @param angle The twist angle of the wavy road at the current position and time
/// @param radius The radius of the sphere to check
/// @param t The time parameter used in the wave angle calculation (twice the elapsed race time)
/// @param relPos The position of the sphere relative to the wavy road's position
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param push Whether to push the collision entry to the @ref CollisionDirector
/// @return Whether a wall collision was detected
/// @details
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectTwistedWay::checkWallCollision(f32 angle, f32 radius, u32 t, const EGG::Vector3f &relPos,
        T *info, KCLTypeMask *maskOut, bool push) {
    auto [sin, cos] = EGG::Mathf::SinCosFIdx(RAD2FIDX * angle);
    f32 yProj = sin * (relPos.y - HALF_WIDTH * 0.5f);
    f32 dist = radius + (yProj - cos * (relPos.x + HALF_WIDTH));

    EGG::Vector3f bbox = EGG::Vector3f::zero;
    EGG::Vector3f fnrm = EGG::Vector3f::zero;

    if (dist <= 0.0f) {
        dist = radius - (yProj - cos * (relPos.x - HALF_WIDTH));

        if (dist > 0.0f) {
            bbox = EGG::Vector3f(-dist * cos, dist * sin, 0.0f);
            fnrm = EGG::Vector3f(-cos, sin, 0.0f);
        }
    } else {
        bbox = EGG::Vector3f(dist * cos, dist * sin, 0.0f);
        fnrm = EGG::Vector3f(cos, sin, 0.0f);
    }

    if (dist > 0.0f) {
        if (info) {
            info->bbox.min = info->bbox.min.minimize(bbox);
            info->bbox.max = info->bbox.max.maximize(bbox);

            if constexpr (std::is_same_v<T, CollisionInfo>) {
                info->updateWall(dist, fnrm);
            }
        }

        if (maskOut) {
            if (push) {
                CollisionDirector::Instance()->pushCollisionEntry(dist, maskOut,
                        KCL_TYPE_BIT(COL_TYPE_WALL), COL_TYPE_WALL);
            } else {
                *maskOut |= KCL_TYPE_BIT(COL_TYPE_WALL);
            }
        }

        return true;
    }

    angle = CalcWaveAngle(0.0f, t);

    EGG::Vector3f wnrm;

    if (!checkPoleCollision(radius, angle, relPos, bbox, wnrm, dist)) {
        return false;
    }

    if (info) {
        info->bbox.min = info->bbox.min.minimize(bbox);
        info->bbox.max = info->bbox.max.maximize(bbox);

        if constexpr (std::is_same_v<T, CollisionInfo>) {
            info->updateWall(dist, wnrm);
        }
    }

    if (maskOut) {
        if (push) {
            CollisionDirector::Instance()->pushCollisionEntry(dist, maskOut,
                    KCL_TYPE_BIT(COL_TYPE_WALL), COL_TYPE_WALL);
        } else {
            *maskOut |= KCL_TYPE_BIT(COL_TYPE_WALL);
        }
    }

    return true;
}

/// @brief Helper function which contains frequently re-used code for checking floor collisions
/// @tparam T The type of collision information structure (@ref CollisionInfo or @ref
/// CollisionInfoPartial)
/// @param angle The twist angle of the wavy road at the current position and time
/// @param radius The radius of the sphere to check
/// @param relPos The position of the sphere relative to the wavy road's position
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param push Whether to push the collision entry to the @ref CollisionDirector
/// @return Whether a wall collision was detected
/// @details Takes the cosine and sine of the provided twist `angle` and computes the distance from
/// the sphere to the floor. If the sphere is below the floor, then returns `false` early.
/// Otherwise, updates the collision information and KCL flags and returns `true`. If `maskOut` is
/// provided, the KCL flags are updated accordingly. If `push` is `true`, then pushes the collision
/// entry to the @ref CollisionDirector.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectTwistedWay::checkFloorCollision(f32 angle, f32 radius, const EGG::Vector3f &relPos,
        T *info, KCLTypeMask *maskOut, bool push) {
    constexpr f32 QUARTER_WIDTH = HALF_WIDTH * -0.5f;
    constexpr f32 TRICKABLE_RADIUS_FACTOR = 0.6f;

    auto [sin, cos] = EGG::Mathf::SinCosFIdx(RAD2FIDX * angle);
    f32 dist = QUARTER_WIDTH + ((radius + -relPos.x * sin) + cos * (-relPos.y - QUARTER_WIDTH));

    if (dist <= 0.0f) {
        return false;
    }

    EGG::Vector3f bbox = dist * EGG::Vector3f::ey;
    EGG::Vector3f fnrm = EGG::Vector3f(sin, cos, 0.0f);

    if (info) {
        info->bbox.min = info->bbox.min.minimize(bbox);
        info->bbox.max = info->bbox.max.maximize(bbox);

        if constexpr (std::is_same_v<T, CollisionInfo>) {
            info->updateFloor(dist, fnrm);
        }
    }

    if (maskOut) {
        if (push) {
            auto *colDir = CollisionDirector::Instance();
            colDir->pushCollisionEntry(dist, maskOut, KCL_TYPE_BIT(COL_TYPE_ROAD), COL_TYPE_ROAD);
            colDir->setCurrentCollisionVariant(2);

            if (EGG::Mathf::abs(relPos.z) < HALF_DEPTH * TRICKABLE_RADIUS_FACTOR) {
                colDir->setCurrentCollisionTrickable(true);
            }
        } else {
            *maskOut |= KCL_TYPE_BIT(COL_TYPE_ROAD);
        }
    }

    return true;
}

/// @addr{0x80814270}
/// @brief Checks for collisions with the pole in the center of the wavy road
/// @param radius The radius of the sphere to check
/// @param angle The twist angle of the wavy road at the current position and time
/// @param relPos The position of the sphere relative to the wavy road's position
/// @param tangentOff The vector to push the sphere away from the collision with the pole
/// @param wnrm The normal vector of the pole at the point of collision
/// @param dist The distance from the sphere to the pole at the point of collision
/// @return `true` if a collision with the pole occurred, `false` otherwise
/// @details Computes the sine and cosine of the provided `angle`, storing them the X and Y
/// component of a vector respectively. Computes the wall normal based off the kart's relative
/// position. Comptues the kart's distance to the pole. If the kart's distance is less than the sum
/// of the pole's radius and the sphere's radius, a collision is detected and `tangentOff` and
/// `dist` are updated accordingly.
bool ObjectTwistedWay::checkPoleCollision(f32 radius, f32 angle, const EGG::Vector3f &relPos,
        EGG::Vector3f &tangentOff, EGG::Vector3f &wnrm, f32 &dist) {
    constexpr EGG::Vector3f PIVOT = EGG::Vector3f(0.0f, 0.5f * HALF_WIDTH, 0.0f);
    constexpr f32 POLE_RADIUS = 227.0f;

    auto [sin, cos] = EGG::Mathf::SinCosFIdx(angle * RAD2FIDX);
    EGG::Vector3f sinCos = EGG::Vector3f(sin, cos, 0.0f);

    wnrm = relPos - (sinCos * (relPos - PIVOT).dot(sinCos) + PIVOT);
    f32 diff = POLE_RADIUS + radius - wnrm.length();

    if (diff <= 0.0f) {
        return false;
    }

    wnrm.normalise2();
    tangentOff = wnrm * diff;
    dist = diff;

    return true;
}

// Explicit instantiation, since callers of checkSphereImpl() live in the header and would
// otherwise be unable to see this definition when the class's vtable is emitted.
template bool ObjectTwistedWay::checkSphereImpl<CollisionInfo>(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset, bool push);
template bool ObjectTwistedWay::checkSphereImpl<CollisionInfoPartial>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset, bool push);

} // namespace Kinoko::Field
