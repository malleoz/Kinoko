#include "CollisionDirector.hh"

#include "game/field/ObjectDrivableDirector.hh"

namespace Kinoko::Field {

/// @addr{0x8078E4F0}
/// @brief Narrows the spatial cache of the @ref CourseColMgr and @ref ObjectDrivableDirector to
/// only include KCL tris defined by the provided mask within a certain radius of the given
/// position.
void CollisionDirector::checkCourseColNarrScLocal(f32 radius, const EGG::Vector3f &pos,
        KCLTypeMask mask, u32 timeOffset) {
    CourseColMgr::Instance()->scaledNarrowScopeLocal(1.0f, radius, nullptr, pos, mask);
    ObjectDrivableDirector::Instance()->colNarScLocal(radius, pos, mask, timeOffset);
}

/// @addr{0x8078F320}
/// @brief Checks collision between a sphere and course KCL and object collision, writing partial
/// collision info. Additionally pushes the collision entry into the CollisionDirector's cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *info,
        KCLTypeMask *typeMaskOut, u32 timeOffset) {
    if (info) {
        info->bbox.setZero();
    }

    if (typeMaskOut) {
        *typeMaskOut = KCL_NONE;
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = flags &&
            courseColMgr->checkSpherePartialPush(1.0f, radius, nullptr, pos, prevPos, flags, info,
                    typeMaskOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSpherePartialPush(radius, pos, prevPos,
            flags, info, typeMaskOut, timeOffset);

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
bool CollisionDirector::checkSphereFull(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut,
        u32 timeOffset) {
    if (pInfo) {
        pInfo->reset();
    }

    if (pFlagsOut) {
        *pFlagsOut = KCL_NONE;
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = flags &&
            courseColMgr->checkSphereFull(1.0f, radius, nullptr, v0, v1, flags, pInfo, pFlagsOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSphereFull(radius, v0, v1, flags, pInfo,
            pFlagsOut, timeOffset);

    if (colliding) {
        if (pInfo) {
            pInfo->tangentOff = pInfo->bbox.min + pInfo->bbox.max;
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
/// collision info. Additionally pushes the collision entry into the CollisionDirector's cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSphereFullPush(f32 radius, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut,
        u32 timeOffset) {
    if (pInfo) {
        pInfo->reset();
    }

    if (pFlagsOut) {
        resetCollisionEntries(pFlagsOut);
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = flags &&
            courseColMgr->checkSphereFullPush(1.0f, radius, nullptr, v0, v1, flags, pInfo,
                    pFlagsOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSphereFullPush(radius, v0, v1, flags,
            pInfo, pFlagsOut, timeOffset);

    if (colliding) {
        if (pInfo) {
            pInfo->tangentOff = pInfo->bbox.min + pInfo->bbox.max;
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
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask typeMask, CollisionInfoPartial *info,
        KCLTypeMask *typeMaskOut, u32 timeOffset) {
    if (info) {
        info->bbox.setZero();
    }

    if (typeMaskOut) {
        *typeMaskOut = KCL_NONE;
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = courseColMgr->checkSphereCachedPartial(1.0f, radius, nullptr, pos, prevPos,
            typeMask, info, typeMaskOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSphereCachedPartial(radius, pos, prevPos,
            typeMask, info, typeMaskOut, timeOffset);

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
/// the collision entry into the CollisionDirector's cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask typeMask, CollisionInfoPartial *info,
        KCLTypeMask *typeMaskOut, u32 timeOffset) {
    if (info) {
        info->bbox.setZero();
    }

    if (typeMaskOut) {
        resetCollisionEntries(typeMaskOut);
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *noBounceInfo = courseColMgr->noBounceWallInfo();
    if (noBounceInfo) {
        noBounceInfo->bbox.setZero();
        noBounceInfo->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = courseColMgr->checkSphereCachedPartialPush(1.0f, radius, nullptr, pos, prevPos,
            typeMask, info, typeMaskOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSphereCachedPartialPush(radius, pos,
            prevPos, typeMask, info, typeMaskOut, timeOffset);

    courseColMgr->clearNoBounceWallInfo();

    return colliding;
}

/// @addr{0x807907F8}
/// @brief Checks collision between a sphere and course KCL and object collision by using the
/// collision director's local spatial cache, writing out full collision info. Additionally pushes
/// the collision entry into the CollisionDirector's cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param timeOffset Optional time delta
/// @return Whether a collision was detected
bool CollisionDirector::checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask typeMask, CollisionInfo *colInfo,
        KCLTypeMask *typeMaskOut, u32 timeOffset) {
    if (colInfo) {
        colInfo->reset();
    }

    if (typeMaskOut) {
        resetCollisionEntries(typeMaskOut);
    }

    auto *courseColMgr = CourseColMgr::Instance();
    auto *info = courseColMgr->noBounceWallInfo();
    if (info) {
        info->bbox.setZero();
        info->dist = std::numeric_limits<f32>::min();
    }

    bool colliding = courseColMgr->checkSphereCachedFullPush(1.0f, radius, nullptr, pos, prevPos,
            typeMask, colInfo, typeMaskOut);

    colliding |= ObjectDrivableDirector::Instance()->checkSphereCachedFullPush(radius, pos, prevPos,
            typeMask, colInfo, typeMaskOut, timeOffset);

    if (colliding) {
        if (colInfo) {
            colInfo->tangentOff = colInfo->bbox.min + colInfo->bbox.max;
        }

        if (info) {
            info->tangentOff = info->bbox.min + info->bbox.max;
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
CollisionDirector::CollisionDirector() {
    m_collisionEntryCount = 0;
    m_closestCollisionEntry = nullptr;
    CourseColMgr::CreateInstance()->init();
}

/// @addr{0x8078E454}
CollisionDirector::~CollisionDirector() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("CollisionDirector instance not explicitly handled!");
    }

    CourseColMgr::DestroyInstance();
}

CollisionDirector *CollisionDirector::s_instance = nullptr; ///< @addr{0x809C2F44}

} // namespace Kinoko::Field
