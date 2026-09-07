#include "CollisionDirector.hh"

namespace Kinoko::Field {

/// @addr{0x8078F320}
/// @brief Checks collision between a sphere and course KCL and object collision, writing partial
/// collision info. Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    if (info) {
        info->bbox.setZero();
    }

    if (maskOut) {
        *maskOut = KCL_NONE;
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = mask &&
            courseColMgr->checkSpherePartialPush(1.0f, radius, nullptr, pos, prevPos, mask, info,
                    maskOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSpherePartialPush(radius, pos, prevPos,
            mask, info, maskOut, timeOffset);

    if (colliding) {
        if (info) {
            info->tangentOff = info->bbox.min + info->bbox.max;
        }

        if (noBounceInfo) {
            noBounceInfo->tangentOff = noBounceInfo->bbox.min + noBounceInfo->bbox.max;
        }
    }

    courseColMgr->clearNoBounceWallInfo();

    return colliding;
}

/// @addr{0x8078F500}
/// @brief Checks collision between a sphere and course KCL and object collision, writing out full
/// collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSphereFull(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    if (info) {
        info->reset();
    }

    if (maskOut) {
        *maskOut = KCL_NONE;
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = mask &&
            courseColMgr->checkSphereFull(1.0f, radius, nullptr, pos, prevPos, mask, info, maskOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSphereFull(radius, pos, prevPos, mask,
            info, maskOut, timeOffset);

    if (colliding) {
        if (info) {
            info->tangentOff = info->bbox.min + info->bbox.max;
        }

        if (noBounceInfo) {
            noBounceInfo->tangentOff = noBounceInfo->bbox.min + noBounceInfo->bbox.max;
        }
    }

    courseColMgr->clearNoBounceWallInfo();

    return colliding;
}

/// @addr{0x8078F784}
/// @brief Checks collision between a sphere and course KCL and object collision, writing out full
/// collision info. Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    if (info) {
        info->reset();
    }

    if (maskOut) {
        resetCollisionEntries(maskOut);
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = mask &&
            courseColMgr->checkSphereFullPush(1.0f, radius, nullptr, pos, prevPos, mask, info,
                    maskOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSphereFullPush(radius, pos, prevPos, mask,
            info, maskOut, timeOffset);

    if (colliding) {
        if (info) {
            info->tangentOff = info->bbox.min + info->bbox.max;
        }

        if (noBounceInfo) {
            noBounceInfo->tangentOff = noBounceInfo->bbox.min + noBounceInfo->bbox.max;
        }
    }

    courseColMgr->clearNoBounceWallInfo();

    return colliding;
}

/// @addr{0x807901F0}
/// @brief Checks collision between a sphere and course KCL and object collision by using the
/// collision director's local spatial cache, writing only partial collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    if (info) {
        info->bbox.setZero();
    }

    if (maskOut) {
        *maskOut = KCL_NONE;
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = courseColMgr->checkSphereCachedPartial(1.0f, radius, nullptr, pos, prevPos,
            mask, info, maskOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSphereCachedPartial(radius, pos, prevPos,
            mask, info, maskOut, timeOffset);

    if (colliding) {
        if (info) {
            info->tangentOff = info->bbox.min + info->bbox.max;
        }

        if (noBounceInfo) {
            noBounceInfo->tangentOff = noBounceInfo->bbox.min + noBounceInfo->bbox.max;
        }
    }

    courseColMgr->clearNoBounceWallInfo();

    return colliding;
}

/// @addr{0x807903BC}
/// @brief Checks collision between a sphere and course KCL and object collision by using the
/// collision director's local spatial cache, writing partial collision info. Additionally pushes
/// the collision entry into the @ref CollisionDirector cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 timeOffset) {
    if (info) {
        info->bbox.setZero();
    }

    if (maskOut) {
        resetCollisionEntries(maskOut);
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = courseColMgr->checkSphereCachedPartialPush(1.0f, radius, nullptr, pos, prevPos,
            mask, info, maskOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSphereCachedPartialPush(radius, pos,
            prevPos, mask, info, maskOut, timeOffset);

    courseColMgr->clearNoBounceWallInfo();

    return colliding;
}

/// @addr{0x807907F8}
/// @brief Checks collision between a sphere and course KCL and object collision by using the
/// collision director's local spatial cache, writing out full collision info. Additionally pushes
/// the collision entry into the @ref CollisionDirector cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 timeOffset) {
    if (info) {
        info->reset();
    }

    if (maskOut) {
        resetCollisionEntries(maskOut);
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = courseColMgr->checkSphereCachedFullPush(1.0f, radius, nullptr, pos, prevPos,
            mask, info, maskOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSphereCachedFullPush(radius, pos, prevPos,
            mask, info, maskOut, timeOffset);

    if (colliding) {
        if (info) {
            info->tangentOff = info->bbox.min + info->bbox.max;
        }

        if (noBounceInfo) {
            noBounceInfo->tangentOff = noBounceInfo->bbox.min + noBounceInfo->bbox.max;
        }
    }

    courseColMgr->clearNoBounceWallInfo();

    return colliding;
}

/// @brief Called when we find a piece of collision we are touching and want to save it temporarily.
/// @addr{0x807BDA9C}
/// @param dist Distance from player to the KCL triangle center
/// @param typeMask Updated to include kclTypeBit
/// @param kclTypeBit The base type of the tri we are colliding with
/// @param attribute The attribute and additional info about the tri we are colliding with
void CollisionDirector::pushCollisionEntry(f32 dist, KCLTypeMask *typeMask, KCLTypeMask kclTypeBit,
        CollisionAttribute attribute) {
    *typeMask = *typeMask | kclTypeBit;
    if (m_collisionEntryCount >= m_entries.size()) {
        m_collisionEntryCount = m_entries.size() - 1;
    }

    m_entries[m_collisionEntryCount++] = CollisionEntry(kclTypeBit, attribute, dist);
}

/// @brief Finds the closest KCL triangle out of the list of tris we are colliding with
/// @addr{0x807BD96C}
/// @param type Filters the result for particular KCL types
/// @return Whether there was a collision entry for the provided type
bool CollisionDirector::findClosestCollisionEntry(KCLTypeMask * /*typeMask*/, KCLTypeMask type) {
    m_closestCollisionEntry = nullptr;
    f32 minDist = -std::numeric_limits<f32>::min();

    for (size_t i = 0; i < m_collisionEntryCount; ++i) {
        const auto &entry = m_entries[i];
        u32 typeMask = entry.typeMask & type;
        if (typeMask != 0 && entry.dist > minDist) {
            minDist = entry.dist;
            m_closestCollisionEntry = &entry;
        }
    }

    return !!m_closestCollisionEntry;
}

/// @addr{0x8078E33C}
/// @brief Private constructor
CollisionDirector::CollisionDirector() {
    m_collisionEntryCount = 0;
    m_closestCollisionEntry = nullptr;
    CourseColMgr::CreateInstance()->init();
}

/// @addr{0x8078E454}
/// @brief Private destructor
CollisionDirector::~CollisionDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("CollisionDirector instance not explicitly handled!");
    }

    CourseColMgr::DestroyInstance();
}

CollisionDirector *CollisionDirector::s_instance = nullptr;

} // namespace Kinoko::Field
