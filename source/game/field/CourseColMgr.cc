#include "CourseColMgr.hh"

#include "game/field/CollisionDirector.hh"

// Credit: em-eight/mkw

namespace Kinoko::Field {

/// @addr{0x807C2A60}
/// @brief Checks collision between a point and course KCL tris, writing only partial collision info
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkPointPartial(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevpos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupPoint(pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithPartialInfo(data, &KColData::checkPointCollision, info, maskOut);
    }

    return doCheckMaskOnly(data, &KColData::checkPointCollision, maskOut);
}

/// @addr{0x807C2DA0}
/// @brief Checks collision between a point and course KCL tris, writing only partial collision
/// info. Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkPointPartialPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevpos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupPoint(pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithPartialInfoPush(data, &KColData::checkPointCollision, info, maskOut);
    }
    return doCheckMaskOnlyPush(data, &KColData::checkPointCollision, maskOut);
}

/// @addr{0x807C30E0}
/// @brief Checks collision between a point and course KCL tris, writing out full collision info
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkPointFull(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevpos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupPoint(pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithFullInfo(data, &KColData::checkPointCollision, info, maskOut);
    }
    return doCheckMaskOnly(data, &KColData::checkPointCollision, maskOut);
}

/// @addr{0x807C3554}
/// @brief Checks collision between a point and course KCL tris, writing out full collision info.
/// Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkPointFullPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevpos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupPoint(pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithFullInfoPush(data, &KColData::checkPointCollision, info, maskOut);
    }
    return doCheckMaskOnlyPush(data, &KColData::checkPointCollision, maskOut);
}

/// @addr{0x807C39C8}
/// @brief Checks collision between a sphere and course KCL tris, writing partial collision info
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param radius The radius of the sphere to check
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkSpherePartial(f32 scale, f32 radius, KColData *data,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevpos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupSphere(radius * invScale, pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithPartialInfo(data, &KColData::checkSphereCollision, info, maskOut);
    }
    return doCheckMaskOnly(data, &KColData::checkSphereCollision, maskOut);
}

/// @addr{0x807C3B5C}
/// @brief Checks collision between a sphere and course KCL tris, writing partial collision info.
/// Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param radius The radius of the sphere to check
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkSpherePartialPush(f32 scale, f32 radius, KColData *data,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevpos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupSphere(radius * invScale, pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithPartialInfoPush(data, &KColData::checkSphereCollision, info, maskOut);
    }
    return doCheckMaskOnlyPush(data, &KColData::checkSphereCollision, maskOut);
}

/// @addr{0x807C3CF0}
/// @brief Checks collision between a sphere and course KCL tris, writing full collision info
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param radius The radius of the sphere to check
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkSphereFull(f32 scale, f32 radius, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevpos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupSphere(radius * invScale, pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithFullInfo(data, &KColData::checkSphereCollision, info, maskOut);
    }
    return doCheckMaskOnly(data, &KColData::checkSphereCollision, maskOut);
}

/// @addr{0x807C3E84}
/// @brief Checks collision between a sphere and course KCL tris, writing full collision info.
/// Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param radius The radius of the sphere to check
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkSphereFullPush(f32 scale, f32 radius, KColData *data,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevpos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupSphere(radius * invScale, pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithFullInfoPush(data, &KColData::checkSphereCollision, info, maskOut);
    }
    return doCheckMaskOnlyPush(data, &KColData::checkSphereCollision, maskOut);
}

/// @addr{0x807C4018}
/// @brief Checks collision between a point and course KCL tris by using the collision director's
/// local spatial cache, writing only partial collision info
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkPointCachedPartial(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevpos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupPoint(pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithPartialInfo(data, &KColData::checkPointCollision, info, maskOut);
    }
    return doCheckMaskOnly(data, &KColData::checkPointCollision, maskOut);
}

/// @addr{0x807C41A4}
/// @brief Checks collision between a point and course KCL tris by using the collision director's
/// local spatial cache, writing only partial collision info. Additionally pushes the collision
/// entry into the @ref CollisionDirector cache.
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkPointCachedPartialPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevpos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    if (data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupPoint(pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithPartialInfoPush(data, &KColData::checkPointCollision, info, maskOut);
    }
    return doCheckMaskOnlyPush(data, &KColData::checkPointCollision, maskOut);
}

/// @addr{0x807C4330}
/// @brief Checks collision between a point and course KCL tris by using the collision director's
/// local spatial cache, writing out full collision info
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkPointCachedFull(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevpos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    if (data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupPoint(pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithFullInfo(data, &KColData::checkPointCollision, info, maskOut);
    }
    return doCheckMaskOnly(data, &KColData::checkPointCollision, maskOut);
}

/// @addr{0x807C44BC}
/// @brief Checks collision between a point and course KCL tris by using the collision director's
/// local spatial cache, writing out full collision info. Additionally pushes the collision entry
/// into the @ref CollisionDirector cache.
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkPointCachedFullPush(f32 scale, KColData *data, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevpos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    if (data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupPoint(pos * invScale, prevpos * invScale, mask);

    if (info) {
        return doCheckWithFullInfoPush(data, &KColData::checkPointCollision, info, maskOut);
    }
    return doCheckMaskOnlyPush(data, &KColData::checkPointCollision, maskOut);
}

/// @addr{0x807C4648}
/// @brief Checks collision between a sphere and course KCL tris by using the collision director's
/// local spatial cache, writing partial collision info
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param radius The radius of the sphere to check
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkSphereCachedPartial(f32 scale, f32 radius, KColData *data,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    if (data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupSphereCached(pos * invScale, prevPos * invScale, mask, radius * invScale);

    if (info) {
        return doCheckWithPartialInfo(data, &KColData::checkSphereCollision, info, maskOut);
    }

    return doCheckMaskOnly(data, &KColData::checkSphereCollision, maskOut);
}

/// @addr{0x807C47F0}
/// @brief Checks collision between a sphere and course KCL tris by using the collision director's
/// local spatial cache, writing partial collision info. Additionally pushes the collision entry
/// into the @ref CollisionDirector cache.
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param radius The radius of the sphere to check
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkSphereCachedPartialPush(f32 scale, f32 radius, KColData *data,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    if (data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupSphereCached(pos * invScale, prevPos * invScale, mask, radius * invScale);

    if (info) {
        return doCheckWithPartialInfoPush(data, &KColData::checkSphereCollision, info, maskOut);
    }

    return doCheckMaskOnlyPush(data, &KColData::checkSphereCollision, maskOut);
}

/// @addr{0x807C4998}
/// @brief Checks collision between a sphere and course KCL tris by using the collision director's
/// local spatial cache, writing full collision info
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param radius The radius of the sphere to check
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkSphereCachedFull(f32 scale, f32 radius, KColData *data,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    if (data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupSphereCached(pos * invScale, prevPos * invScale, mask, radius * invScale);

    if (info) {
        return doCheckWithFullInfo(data, &KColData::checkSphereCollision, info, maskOut);
    }

    return doCheckMaskOnly(data, &KColData::checkSphereCollision, maskOut);
}

/// @addr{0x807C4B40}
/// @brief Checks collision between a sphere and course KCL tris by using the collision director's
/// local spatial cache, writing full collision info. Additionally pushes the collision entry into
/// the @ref CollisionDirector cache.
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param radius The radius of the sphere to check
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param pos The position of the sphere to check
/// @param prevpos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::checkSphereCachedFullPush(f32 scale, f32 radius, KColData *data,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut) {
    if (!data) {
        data = m_data;
    }

    if (data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupSphereCached(pos * invScale, prevPos * invScale, mask, radius * invScale);

    if (info) {
        return doCheckWithFullInfoPush(data, &KColData::checkSphereCollision, info, maskOut);
    }

    return doCheckMaskOnlyPush(data, &KColData::checkSphereCollision, maskOut);
}

/// @addr{0x807C29E4}
/// @brief Private constructor
CourseColMgr::CourseColMgr()
    : m_data(nullptr), m_kclScale(1.0f), m_noBounceWallInfo(nullptr), m_localMtx(nullptr) {}

/// @addr{0x807C2A04}
/// @brief Private destructor
CourseColMgr::~CourseColMgr() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("CourseColMgr instance not explicitly handled!");
    }

    ASSERT(m_data);
    EGG::egg_delete(m_data);
}

/// @addr{0x807C2BD8}
/// @brief Calls into the provided KColData query function, accumulates soft wall collision info,
/// accumulates the colliding base type flags, and accumulates partial collision info for solid
/// surfaces.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param collisionCheckFunc The KColData query function to call
/// @param info Out parameter for retrieving partial collision information (if any)
/// @param typeMask The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::doCheckWithPartialInfo(KColData *data, CollisionCheckFunc collisionCheckFunc,
        CollisionInfoPartial *info, KCLTypeMask *typeMask) {
    f32 dist;
    EGG::Vector3f fnrm;
    u16 attribute;
    bool hasCol = false;

    while ((data->*collisionCheckFunc)(&dist, &fnrm, &attribute)) {
        hasCol = true;
        dist *= m_kclScale;

        if (m_noBounceWallInfo && (attribute & KCL_SOFT_WALL_MASK)) {
            if (m_localMtx) {
                fnrm = m_localMtx->multVector33(fnrm);
            }
            EGG::Vector3f offset = fnrm * dist;
            m_noBounceWallInfo->bbox.min = m_noBounceWallInfo->bbox.min.minimize(offset);
            m_noBounceWallInfo->bbox.max = m_noBounceWallInfo->bbox.max.maximize(offset);
            if (m_noBounceWallInfo->dist < dist) {
                m_noBounceWallInfo->dist = dist;
                m_noBounceWallInfo->fnrm = fnrm;
            }
        } else {
            u32 flags = KCL_ATTRIBUTE_TYPE_BIT(attribute);
            if (typeMask) {
                *typeMask = *typeMask | flags;
            }
            if (flags & KCL_TYPE_SOLID_SURFACE) {
                EGG::Vector3f offset = fnrm * dist;
                info->bbox.min = info->bbox.min.minimize(offset);
                info->bbox.max = info->bbox.max.maximize(offset);
            }
        }
    }

    m_localMtx = nullptr;

    return hasCol;
}

/// @addr{0x807C2F18}
/// @brief Calls into the provided KColData query function, accumulates soft wall collision info,
/// accumulates the colliding base type flags, and accumulates partial collision info for solid
/// surfaces. Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param collisionCheckFunc The KColData query function to call
/// @param info Out parameter for retrieving partialcollision information (if any)
/// @param typeMask The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::doCheckWithPartialInfoPush(KColData *data, CollisionCheckFunc collisionCheckFunc,
        CollisionInfoPartial *info, KCLTypeMask *typeMask) {
    f32 dist;
    EGG::Vector3f fnrm;
    u16 attribute;
    bool hasCol = false;

    while ((data->*collisionCheckFunc)(&dist, &fnrm, &attribute)) {
        hasCol = true;
        dist *= m_kclScale;

        if (!m_noBounceWallInfo || !(attribute & KCL_SOFT_WALL_MASK)) {
            u32 flags = KCL_ATTRIBUTE_TYPE_BIT(attribute);
            if (typeMask) {
                CollisionDirector::Instance()->pushCollisionEntry(dist, typeMask, flags, attribute);
            }
            if (flags & KCL_TYPE_SOLID_SURFACE) {
                EGG::Vector3f offset = fnrm * dist;
                info->bbox.min = info->bbox.min.minimize(offset);
                info->bbox.max = info->bbox.max.maximize(offset);
            }
        } else {
            if (m_localMtx) {
                fnrm = m_localMtx->multVector33(fnrm);
            }
            EGG::Vector3f offset = fnrm * dist;
            m_noBounceWallInfo->bbox.min = m_noBounceWallInfo->bbox.min.minimize(offset);
            m_noBounceWallInfo->bbox.max = m_noBounceWallInfo->bbox.max.maximize(offset);
            if (m_noBounceWallInfo->dist < dist) {
                m_noBounceWallInfo->dist = dist;
                m_noBounceWallInfo->fnrm = fnrm;
            }
        }
    }

    m_localMtx = nullptr;

    return hasCol;
}

/// @addr{0x807C3258}
/// @brief Calls into the provided KColData query function, accumulates soft wall collision info,
/// accumulates the colliding base type flags, and accumulates full collision info for solid
/// surfaces.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param collisionCheckFunc The KColData query function to call
/// @param info Out parameter for retrieving collision information (if any)
/// @param flagsOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::doCheckWithFullInfo(KColData *data, CollisionCheckFunc collisionCheckFunc,
        CollisionInfo *info, KCLTypeMask *flagsOut) {
    f32 dist;
    EGG::Vector3f fnrm;
    u16 attribute;
    bool hasCol = false;

    while ((data->*collisionCheckFunc)(&dist, &fnrm, &attribute)) {
        dist *= m_kclScale;

        if (m_noBounceWallInfo && attribute & KCL_SOFT_WALL_MASK) {
            if (m_localMtx) {
                fnrm = m_localMtx->multVector33(fnrm);
            }
            EGG::Vector3f offset = fnrm * dist;
            m_noBounceWallInfo->bbox.min = m_noBounceWallInfo->bbox.min.minimize(offset);
            m_noBounceWallInfo->bbox.max = m_noBounceWallInfo->bbox.max.maximize(offset);
            if (m_noBounceWallInfo->dist < dist) {
                m_noBounceWallInfo->dist = dist;
                m_noBounceWallInfo->fnrm = fnrm;
            }
        } else {
            u32 kclAttributeTypeBit = KCL_ATTRIBUTE_TYPE_BIT(attribute);
            if (flagsOut) {
                *flagsOut |= kclAttributeTypeBit;
            }
            if (kclAttributeTypeBit & KCL_TYPE_SOLID_SURFACE) {
                info->update(dist, fnrm * dist, fnrm, kclAttributeTypeBit);
            }
        }

        hasCol = true;
    }

    m_localMtx = nullptr;

    return hasCol;
}

/// @addr{0x807C36CC}
/// @brief Calls into the provided KColData query function, accumulates soft wall collision info,
/// accumulates the colliding base type flags, and accumulates full collision info for solid
/// surfaces. Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param collisionCheckFunc The KColData query function to call
/// @param info Out parameter for retrieving collision information (if any)
/// @param flagsOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::doCheckWithFullInfoPush(KColData *data, CollisionCheckFunc collisionCheckFunc,
        CollisionInfo *info, KCLTypeMask *flagsOut) {
    f32 dist;
    EGG::Vector3f fnrm;
    u16 attribute;
    bool hasCol = false;

    while ((data->*collisionCheckFunc)(&dist, &fnrm, &attribute)) {
        dist *= m_kclScale;

        if (m_noBounceWallInfo && attribute & KCL_SOFT_WALL_MASK) {
            if (m_localMtx) {
                fnrm = m_localMtx->multVector33(fnrm);
            }
            EGG::Vector3f offset = fnrm * dist;
            m_noBounceWallInfo->bbox.min = m_noBounceWallInfo->bbox.min.minimize(offset);
            m_noBounceWallInfo->bbox.max = m_noBounceWallInfo->bbox.max.maximize(offset);
            if (m_noBounceWallInfo->dist < dist) {
                m_noBounceWallInfo->dist = dist;
                m_noBounceWallInfo->fnrm = fnrm;
            }
        } else {
            u32 kclAttributeTypeBit = KCL_ATTRIBUTE_TYPE_BIT(attribute);
            if (flagsOut) {
                CollisionDirector::Instance()->pushCollisionEntry(dist, flagsOut,
                        kclAttributeTypeBit, attribute);
            }
            if (kclAttributeTypeBit & KCL_TYPE_SOLID_SURFACE) {
                info->update(dist, fnrm * dist, fnrm, kclAttributeTypeBit);
            }
        }

        hasCol = true;
    }

    m_localMtx = nullptr;

    return hasCol;
}

/// @brief Calls into the provided KColData query function, only accumulating the colliding base
/// type flags
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param collisionCheckFunc The KColData query function to call
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool CourseColMgr::doCheckMaskOnly(KColData *data, CollisionCheckFunc collisionCheckFunc,
        KCLTypeMask *maskOut) {
    bool hasCol = false;
    f32 dist;
    u16 attribute;

    while ((data->*collisionCheckFunc)(&dist, nullptr, &attribute)) {
        if ((!m_noBounceWallInfo || !(attribute & KCL_SOFT_WALL_MASK)) && maskOut) {
            *maskOut |= KCL_ATTRIBUTE_TYPE_BIT(attribute);
        }
        hasCol = true;
    }

    return hasCol;
}

/// @brief Calls into the provided KColData query function, only accumulating the colliding base
/// type flags. Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param collisionCheckFunc The KColData query function to call
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected.
bool CourseColMgr::doCheckMaskOnlyPush(KColData *data, CollisionCheckFunc collisionCheckFunc,
        KCLTypeMask *maskOut) {
    bool hasCol = false;
    f32 dist;
    u16 attribute;

    while ((data->*collisionCheckFunc)(&dist, nullptr, &attribute)) {
        if ((!m_noBounceWallInfo || !(attribute & KCL_SOFT_WALL_MASK)) && maskOut) {
            CollisionDirector::Instance()->pushCollisionEntry(dist, maskOut,
                    KCL_ATTRIBUTE_TYPE_BIT(attribute), attribute);
        }
        hasCol = true;
    }

    return hasCol;
}

CourseColMgr *CourseColMgr::s_instance = nullptr;

} // namespace Kinoko::Field
