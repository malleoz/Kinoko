#include "ObjectTwistedWay.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x80813BD4}
ObjectTwistedWay::ObjectTwistedWay(const System::MapdataGeoObj &params) : ObjectDrivable(params) {}

/// @addr{0x80814918}
ObjectTwistedWay::~ObjectTwistedWay() = default;

/// @addr{0x80813CFC}
void ObjectTwistedWay::calc() {
    if (!System::RaceManager::Instance()->isStageReached(System::RaceManager::Stage::Race)) {
        ++m_introTimer;
    }
}

/// @brief Helper function which contains frequently re-used code. Behavior branches depending on
/// whether it is a full or partial check (call CollisionInfo::updateFloor) or push (push entry in
/// the CollisionDirector).
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectTwistedWay::checkSphereImpl(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f & /*v1*/, KCLTypeMask flags, T *pInfo, KCLTypeMask *pFlagsOut,
        u32 timeOffset, bool push) {
    EGG::Vector3f relPos = v0 - pos();

    if (EGG::Mathf::abs(relPos.z) > HALF_DEPTH || EGG::Mathf::abs(relPos.x) > WIDTH * 2.0f) {
        return false;
    }

    auto *raceMgr = System::RaceManager::Instance();
    bool isInRace = raceMgr->isStageReached(System::RaceManager::Stage::Race);

    u32 frameCount = timeOffset + (isInRace ? raceMgr->timer() : m_introTimer);
    u32 t = (frameCount % PERIOD_LENGTH) * 2;

    f32 angle = isInRace ? calcWavePhase(-relPos.z / HALF_DEPTH, t) : 0.0f;

    bool hasCol = false;
    if (flags & KCL_TYPE_BIT(COL_TYPE_WALL)) {
        hasCol |= checkWallCollision(angle, radius, t, relPos, pInfo, pFlagsOut, push);
    }

    if (flags & KCL_TYPE_BIT(COL_TYPE_ROAD)) {
        hasCol |= checkFloorCollision(angle, radius, relPos, pInfo, pFlagsOut, push);
    }

    return hasCol;
}

/// @brief Helper function which contains frequently re-used code for checking wall collisions
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectTwistedWay::checkWallCollision(f32 angle, f32 radius, u32 t, const EGG::Vector3f &relPos,
        T *pInfo, KCLTypeMask *pFlagsOut, bool push) {
    auto [sin, cos] = EGG::Mathf::SinCosFIdx(RAD2FIDX * angle);
    f32 yProj = sin * (relPos.y - WIDTH * 0.5f);
    f32 dist = radius + (yProj - cos * (relPos.x + WIDTH));

    EGG::Vector3f bbox = EGG::Vector3f::zero;
    EGG::Vector3f fnrm = EGG::Vector3f::zero;

    if (dist <= 0.0f) {
        dist = radius - (yProj - cos * (relPos.x - WIDTH));

        if (dist > 0.0f) {
            bbox = EGG::Vector3f(-dist * cos, dist * sin, 0.0f);
            fnrm = EGG::Vector3f(-cos, sin, 0.0f);
        }
    } else {
        bbox = EGG::Vector3f(dist * cos, dist * sin, 0.0f);
        fnrm = EGG::Vector3f(cos, sin, 0.0f);
    }

    if (dist > 0.0f) {
        if (pInfo) {
            pInfo->bbox.min = pInfo->bbox.min.minimize(bbox);
            pInfo->bbox.max = pInfo->bbox.max.maximize(bbox);

            if constexpr (std::is_same_v<T, CollisionInfo>) {
                pInfo->updateWall(dist, fnrm);
            }
        }

        if (pFlagsOut) {
            if (push) {
                CollisionDirector::Instance()->pushCollisionEntry(dist, pFlagsOut,
                        KCL_TYPE_BIT(COL_TYPE_WALL), COL_TYPE_WALL);
            } else {
                *pFlagsOut |= KCL_TYPE_BIT(COL_TYPE_WALL);
            }
        }

        return true;
    }

    angle = calcWavePhase(0.0f, t);

    EGG::Vector3f wnrm;

    if (!checkPoleCollision(radius, angle, relPos, bbox, wnrm, dist)) {
        return false;
    }

    if (pInfo) {
        pInfo->bbox.min = pInfo->bbox.min.minimize(bbox);
        pInfo->bbox.max = pInfo->bbox.max.maximize(bbox);

        if constexpr (std::is_same_v<T, CollisionInfo>) {
            pInfo->updateWall(dist, wnrm);
        }
    }

    if (pFlagsOut) {
        if (push) {
            CollisionDirector::Instance()->pushCollisionEntry(dist, pFlagsOut,
                    KCL_TYPE_BIT(COL_TYPE_WALL), COL_TYPE_WALL);
        } else {
            *pFlagsOut |= KCL_TYPE_BIT(COL_TYPE_WALL);
        }
    }

    return true;
}

/// @brief Helper function which contains frequently re-used code for checking floor collisions
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectTwistedWay::checkFloorCollision(f32 angle, f32 radius, const EGG::Vector3f &relPos,
        T *pInfo, KCLTypeMask *pFlagsOut, bool push) {
    constexpr f32 HALF_WIDTH = WIDTH * -0.5f;
    constexpr f32 TRICKABLE_RADIUS_FACTOR = 0.6f;

    auto [sin, cos] = EGG::Mathf::SinCosFIdx(RAD2FIDX * angle);
    f32 dist = HALF_WIDTH + ((radius + -relPos.x * sin) + cos * (-relPos.y - HALF_WIDTH));

    if (dist <= 0.0f) {
        return false;
    }

    EGG::Vector3f bbox = dist * EGG::Vector3f::ey;
    EGG::Vector3f fnrm = EGG::Vector3f(sin, cos, 0.0f);

    if (pInfo) {
        pInfo->bbox.min = pInfo->bbox.min.minimize(bbox);
        pInfo->bbox.max = pInfo->bbox.max.maximize(bbox);

        if constexpr (std::is_same_v<T, CollisionInfo>) {
            pInfo->updateFloor(dist, fnrm);
        }
    }

    if (pFlagsOut) {
        if (push) {
            auto *colDir = CollisionDirector::Instance();
            colDir->pushCollisionEntry(dist, pFlagsOut, KCL_TYPE_BIT(COL_TYPE_ROAD), COL_TYPE_ROAD);
            colDir->setCurrentCollisionVariant(2);

            if (EGG::Mathf::abs(relPos.z) < HALF_DEPTH * TRICKABLE_RADIUS_FACTOR) {
                colDir->setCurrentCollisionTrickable(true);
            }
        } else {
            *pFlagsOut |= KCL_TYPE_BIT(COL_TYPE_ROAD);
        }
    }

    return true;
}

/// @addr{0x80814270}
/// @brief Checks for collisions with the pole in the center of the wavy road
bool ObjectTwistedWay::checkPoleCollision(f32 radius, f32 angle, const EGG::Vector3f &relPos,
        EGG::Vector3f &v0, EGG::Vector3f &wnrm, f32 &dist) {
    constexpr EGG::Vector3f HEIGHT = EGG::Vector3f(0.0f, 0.5f * WIDTH, 0.0f);
    constexpr f32 POLE_RADIUS = 227.0f;

    auto [sin, cos] = EGG::Mathf::SinCosFIdx(angle * RAD2FIDX);
    EGG::Vector3f sinCos = EGG::Vector3f(sin, cos, 0.0f);

    wnrm = relPos - (sinCos * (relPos - HEIGHT).dot(sinCos) + HEIGHT);
    f32 diff = POLE_RADIUS + radius - wnrm.length();

    if (diff <= 0.0f) {
        return false;
    }

    wnrm.normalise2();
    v0 = wnrm * diff;
    dist = diff;

    return true;
}

/// @addr{0x80813F40}
/// @brief Calculates the phase of the wave at the given time t and position along the z-axis
f32 ObjectTwistedWay::calcWavePhase(f32 zPercent, u32 t) {
    constexpr f32 WAVINESS = 4.0f;
    constexpr f32 AMPLITUDE = 0.2f;

    f32 angle = EGG::Mathf::SinFIdx(
            (static_cast<f32>(t) * F_PI / PERIOD_LENGTH + WAVINESS * zPercent) * RAD2FIDX);

    f32 low = zPercent - 1.0f;
    f32 high = zPercent + 1.0f;

    return AMPLITUDE * (high * (high * (low * (low * angle))));
}

} // namespace Kinoko::Field
