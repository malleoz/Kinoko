#include "ObjectTuribashi.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x80805A4C}
ObjectTuribashi::ObjectTuribashi(const System::MapdataGeoObj &params) : ObjectDrivable(params) {}

/// @addr{0x80806514}
ObjectTuribashi::~ObjectTuribashi() = default;

/// @brief Helper function which contains frequently re-used code. Behavior branches depending on
/// whether it is a full or partial check (call CollisionInfo::updateFloor) or push (push entry in
/// the CollisionDirector).
/// @details Interestingly, this function makes assumptions about the object's rotation. Even if the
/// object is rotated, collision will still act as if it is oriented along the z-axis.
/// @tparam T The CollisionInfo object type, either CollisionInfoPartial or CollisionInfo.
/// @param push Whether to push a collision entry
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectTuribashi::checkSphereImpl(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f & /*v1*/, KCLTypeMask flags, T *pInfo, KCLTypeMask *pFlagsOut,
        u32 timeOffset, bool push) {
    constexpr u16 PERIOD = 160;     // Framecount of a full bridge swing.
    constexpr f32 HEIGHT = 2000.0f; // Distance between min/max positions along the angled bridge.

    // This check normally happens after the stage check,
    // but there's no difference in behavior if we check earlier.
    if ((flags & 1) == 0) {
        return false;
    }

    EGG::Vector3f deltaPos = v0 - pos();

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

    if (pInfo) {
        EGG::Vector3f scaledY = EGG::Vector3f::ey * dist;
        pInfo->bbox.min = pInfo->bbox.min.minimize(scaledY);
        pInfo->bbox.max = pInfo->bbox.max.maximize(scaledY);

        if constexpr (std::is_same_v<T, CollisionInfo>) {
            pInfo->updateFloor(dist, EGG::Vector3f(sinfidx, cosfidx, 0.0f));
        }
    }

    if (pFlagsOut) {
        if (push) {
            auto *colDirector = CollisionDirector::Instance();
            colDirector->pushCollisionEntry(dist, pFlagsOut, KCL_TYPE_BIT(COL_TYPE_ROAD), 0);
            colDirector->setCurrentCollisionVariant(4);
            colDirector->setCurrentCollisionTrickable(false);
        } else {
            *pFlagsOut |= KCL_TYPE_BIT(COL_TYPE_ROAD);
        }
    }

    return true;
}

} // namespace Kinoko::Field
