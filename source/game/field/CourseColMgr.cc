#include "CourseColMgr.hh"

#include "game/field/CollisionDirector.hh"

// Credit: em-eight/mkw

namespace Kinoko::Field {

/// @brief Helper function which contains frequently re-used code for the checkPoint* family of
/// collision check functions
/// @tparam T The collision info object type, either @ref CollisionInfoPartial or @ref
/// CollisionInfo.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param cached Whether the check should use the local spatial cache
/// @param push Whether to push a collision entry into the @ref CollisionDirector cache
/// @return Whether a collision was detected
/// @details If `data` is `nullptr`, the function will use the internal course collision data
/// (`m_data`). If `cached` is true and the local spatial cache is empty, the function will
/// early-return `false`. Sets @ref m_kclScale to the provided `scale` value. Dispatches to @ref
/// KColData::lookupPoint() to prepare for the collision check. Calls the appropriate `doCheck*`
/// function based on whether `info` is `nullptr`.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool CourseColMgr::checkPointImpl(KColData *data, f32 scale, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut, bool cached,
        bool push) {
    if (!data) {
        data = m_data;
    }

    if (cached && data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    data->lookupPoint(pos * invScale, prevPos * invScale, mask);

    if (info) {
        return doCheckWithInfo(data, &KColData::checkPointCollision, info, maskOut, push);
    }

    return doCheckMaskOnly(data, &KColData::checkPointCollision, maskOut, push);
}

/// @brief Shared implementation backing the checkSphere* family of functions
/// @tparam T The collision info object type, either @ref CollisionInfoPartial or @ref
/// CollisionInfo.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param scale Compensates for local-to-world transformation for dyanmically-sized objects
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param cached Whether the check should use the local spatial cache
/// @param push Whether to push a collision entry into the @ref CollisionDirector cache
/// @return Whether a collision was detected
/// @details If `data` is `nullptr`, the function will use the internal course collision data
/// (`m_data`). If `cached` is true and the local spatial cache is empty, the function will
/// early-return `false`. Sets @ref m_kclScale to the provided `scale` value. Dispatches to @ref
/// @ref KColData::lookupSphere() or @ref KColData::lookupSphereCached() depending on whether
/// `cached` is `true` to prepare for the collision check. Calls the appropriate `doCheck*` function
/// based on whether `info` is `nullptr`.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool CourseColMgr::checkSphereImpl(KColData *data, f32 scale, f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut, bool cached,
        bool push) {
    if (!data) {
        data = m_data;
    }

    if (cached && data->prismCache(0) == 0) {
        return false;
    }

    m_kclScale = scale;

    f32 invScale = 1.0f / scale;
    if (cached) {
        data->lookupSphereCached(radius * invScale, pos * invScale, prevPos * invScale, mask);
    } else {
        data->lookupSphere(radius * invScale, pos * invScale, prevPos * invScale, mask);
    }

    if (info) {
        return doCheckWithInfo(data, &KColData::checkSphereCollision, info, maskOut, push);
    }

    return doCheckMaskOnly(data, &KColData::checkSphereCollision, maskOut, push);
}

/// @addr{0x807C2BD8}
/// @brief Calls into the provided KColData query function, accumulating collision info for each
/// collision
/// @tparam T The collision info object type, either @ref CollisionInfoPartial or @ref
/// CollisionInfo
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param collisionCheckFunc The KColData query function to call
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param push Whether to push a collision entry into the @ref CollisionDirector cache
/// @return Whether a collision was detected
/// @details This function repeatedly calls the provided @ref KColData query function until no more
/// collisions are detected. For each collision, scales the collision distance by `m_kclScale` to
/// account for local-to-world transformations.
/// If @ref m_noBounceWallInfo is set and the collision is a soft wall, it updates the soft wall
/// collision info accordingly.
///
/// Otherwise, if `maskOut` is provided, it sets the @ref KColType bits that were hit; it also
/// pushes to the @ref CollisionDirector cache if `push` is `true`. If the colliding flags include
/// #KCL_TYPE_SOLID_SURFACE, `info` is updated accordingly.
///
/// Finally, clears @ref m_localMtx to ensure it does not affect subsequent collision checks.
/// Returns `true` if at least one collision occurred.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool CourseColMgr::doCheckWithInfo(KColData *data, CollisionCheckFunc collisionCheckFunc, T *info,
        KCLTypeMask *maskOut, bool push) {
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
            m_noBounceWallInfo->updateBBox(offset);
            if (m_noBounceWallInfo->dist < dist) {
                m_noBounceWallInfo->dist = dist;
                m_noBounceWallInfo->fnrm = fnrm;
            }
        } else {
            u32 flags = KCL_ATTRIBUTE_TYPE_BIT(attribute);
            if (maskOut) {
                if (push) {
                    CollisionDirector::Instance()->pushCollisionEntry(dist, maskOut, flags,
                            attribute);
                } else {
                    *maskOut = *maskOut | flags;
                }
            }
            if (flags & KCL_TYPE_SOLID_SURFACE) {
                if constexpr (std::is_same_v<T, CollisionInfoPartial>) {
                    EGG::Vector3f offset = fnrm * dist;
                    info->bbox.min = info->bbox.min.minimize(offset);
                    info->bbox.max = info->bbox.max.maximize(offset);
                } else {
                    info->update(dist, fnrm * dist, fnrm, flags);
                }
            }
        }
    }

    m_localMtx = nullptr;

    return hasCol;
}

/// @brief Calls into the provided @ref KColData query function, accumulating the colliding base
/// type flags and conditionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param data Pointer to the parsed tri data to perform the lookup on
/// @param collisionCheckFunc The KColData query function to call
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param push Whether to push a collision entry into the @ref CollisionDirector cache
/// @return Whether a collision was detected
/// @details This function repeatedly calls the provided `collisionCheckFunc` on the provided @ref
/// KColData object. For each collision, it accumulates the colliding @ref KColType flags, and
/// if `push` is `true`, also pushes the collision entry into the @ref CollisionDirector cache. This
/// function skips soft wall collisions if @ref m_noBounceWallInfo is set. Returns `true` if at
/// least one collision occurred.
bool CourseColMgr::doCheckMaskOnly(KColData *data, CollisionCheckFunc collisionCheckFunc,
        KCLTypeMask *maskOut, bool push) {
    bool hasCol = false;
    f32 dist;
    u16 attribute;

    while ((data->*collisionCheckFunc)(&dist, nullptr, &attribute)) {
        if ((!m_noBounceWallInfo || !(attribute & KCL_SOFT_WALL_MASK)) && maskOut) {
            if (push) {
                CollisionDirector::Instance()->pushCollisionEntry(dist, maskOut,
                        KCL_ATTRIBUTE_TYPE_BIT(attribute), attribute);
            } else {
                *maskOut |= KCL_ATTRIBUTE_TYPE_BIT(attribute);
            }
        }
        hasCol = true;
    }

    return hasCol;
}

CourseColMgr *CourseColMgr::s_instance = nullptr;

// Explicit instantiation, since callers of checkPointImpl()/checkSphereImpl() live in the header
// and would otherwise be unable to see these definitions.
template bool CourseColMgr::checkPointImpl<CollisionInfo>(KColData *data, f32 scale,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut, bool cached, bool push);
template bool CourseColMgr::checkPointImpl<CollisionInfoPartial>(KColData *data, f32 scale,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut, bool cached, bool push);
template bool CourseColMgr::checkSphereImpl<CollisionInfo>(KColData *data, f32 scale, f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut, bool cached, bool push);
template bool CourseColMgr::checkSphereImpl<CollisionInfoPartial>(KColData *data, f32 scale,
        f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut, bool cached, bool push);

} // namespace Kinoko::Field
