#include "ObjectTuribashi.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief Helper function which contains frequently re-used code. Behavior branches depending on
/// whether it is a full or partial check (call CollisionInfo::updateFloor) or push (push entry in
/// the CollisionDirector).
/// @tparam T The CollisionInfo object type, either CollisionInfoPartial or CollisionInfo.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param mask The KCL mask to check collision against (other types are ignored)
/// @param info Out parameter of type `T` for retrieving partial collision information (if any)
/// @param maskOut The KCL mask that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @param push Whether to push a collision entry
/// @return `true` if the kart is considered to be colliding with the bridge, `false` otherwise.
/// @details If `mask` does not include #COL_TYPE_ROAD, then this function early returns. In the
/// base game, this check is normally performed after the stage check, but in Kinoko, we take
/// creative liberty to perform the check earlier to avoid unnecessary computation of the bridge's
/// angle in this function. If the kart's position does not intersect with the bridge (determined by
/// comparing the kart's X-position to @ref HALF_WIDTH and Z-position to @ref HALF_LENGTH), the
/// function will also early return.
///
/// The bridge's sway is calculated using a sine and cosine wave. The sine wave is used to
/// calculate the vertical displacement depending on your X and Z position. The sway is
/// amplified as you move towards the X-axis edges of the bridge, and is further maximized as you
/// approach the Z-axis center of the bridge. The cosine wave is used to create "bumps" which make
/// the bridge's planks feel more realistic as you drive along the bridge.
///
/// Computes the kart sphere's overlap distance into the bridge's floor. If this distance is
/// negative or greater than `600.0f`, the kart is considered to not be colliding with the bridge
/// and the function early returns. If the kart's distance is greater than `300.0f`, the kart's
/// overlap distance is dampened to prevent abrupt snapping up to the bridge's floor.
///
/// With the kart's effective distance computed, populates `info` with the collision details. If
/// `push` is true, then the collision entry is pushed to the @ref CollisionDirector. Otherwise,
/// sets the #COL_TYPE_ROAD bit in `maskOut`.
/// @note Interestingly, this function makes assumptions about the object's rotation. Even if the
/// object is rotated, collision will still act as if it is oriented along the z-axis.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectTuribashi::checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
        u32 timeOffset, bool push) {
    constexpr u16 PERIOD = 160;     // Framecount of a full bridge swing.
    constexpr f32 HEIGHT = 2000.0f; // Distance between min/max positions along the angled bridge.

    // This check normally happens after the stage check,
    // but there's no difference in behavior if we check earlier.
    if ((mask & KCL_TYPE_BIT(COL_TYPE_ROAD)) == 0) {
        return false;
    }

    EGG::Vector3f deltaPos = pos - ObjectTuribashi::pos();

    if (EGG::Mathf::abs(deltaPos.z) > HALF_LENGTH || EGG::Mathf::abs(deltaPos.x) > HALF_WIDTH) {
        return false;
    }

    const auto *raceMgr = System::RaceManager::Instance();
    f32 angle = 0.0f;

    if (raceMgr->isStageReached(System::RaceManager::Stage::Race)) {
        f32 phase = -deltaPos.z / HALF_LENGTH;
        f32 lower = phase - 1.0f;
        f32 higher = 1.0f + phase;
        u32 t = (timeOffset + raceMgr->timer()) % PERIOD;
        f32 sin = EGG::Mathf::SinFIdx(RAD2FIDX *
                (F_PI * static_cast<f32>(t * 2) / static_cast<f32>(PERIOD) + 0.7f * phase));
        angle = 0.12f * (higher * (higher * (lower * (lower * sin))));
    }

    auto [sinfidx, cosfidx] = EGG::Mathf::SinCosFIdx(RAD2FIDX * angle);

    f32 phase = deltaPos.z / HALF_LENGTH;
    f32 cos = EGG::Mathf::CosFIdx(RAD2FIDX * (0.5f * (F_PI * (37.0f * phase))));
    f32 base = HEIGHT * -0.5f;
    f32 dist = base +
            (cosfidx * (-deltaPos.y - base) +
                    (-deltaPos.x * sinfidx +
                            (radius + (EGG::Mathf::abs(30.0f * cos) + 25.0f) +
                                    0.156f * deltaPos.z)));

    if (0.0f >= dist || dist >= 600.0f) {
        return false;
    }

    if (300.0f < dist) {
        dist *= 0.2f;
    }

    if (info) {
        EGG::Vector3f scaledY = EGG::Vector3f::ey * dist;
        info->updateBBox(scaledY);

        if constexpr (std::is_same_v<T, CollisionInfo>) {
            info->updateFloor(dist, EGG::Vector3f(sinfidx, cosfidx, 0.0f));
        }
    }

    if (maskOut) {
        if (push) {
            auto *colDirector = CollisionDirector::Instance();
            colDirector->pushCollisionEntry(dist, maskOut, KCL_TYPE_BIT(COL_TYPE_ROAD), 0);
            colDirector->setCurrentCollisionVariant(4);
            colDirector->setCurrentCollisionTrickable(false);
        } else {
            *maskOut |= KCL_TYPE_BIT(COL_TYPE_ROAD);
        }
    }

    return true;
}

// Explicit instantiation, since callers of checkSphereImpl() live in the header and would
// otherwise be unable to see this definition when the class's vtable is emitted.
template bool ObjectTuribashi::checkSphereImpl<CollisionInfo>(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset, bool push);
template bool ObjectTuribashi::checkSphereImpl<CollisionInfoPartial>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset, bool push);

} // namespace Kinoko::Field
