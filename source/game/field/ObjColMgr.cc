#include "ObjColMgr.hh"

#include "game/field/CourseColMgr.hh"

namespace Kinoko::Field {

/// @addr{0x807C4CE8}
/// @brief Constructor that parses the KCL data from the provided file pointer
/// @param file Pointer to the .kcl file in memory
/// @details Initializes the transformation matrix to the identity matrix, sets @ref m_kclScale
/// to `1.0f`, and zeroes @ref m_movingObjVel. Finally, it parses the KCL data from the provided
/// file pointer to @ref m_data.
ObjColMgr::ObjColMgr(const void *file)
    : m_data(EGG::egg_new<KColData>(file)),
      m_mtx(EGG::Matrix34f::ident),
      m_mtxInv(EGG::Matrix34f::ident),
      m_kclScale(1.0f),
      m_movingObjVel(EGG::Vector3f::zero) {}

/// @addr{0x807C4D6C}
/// @brief Destructor that destroys the associated KCL data
ObjColMgr::~ObjColMgr() {
    ASSERT(m_data);
    EGG::egg_delete(m_data);
}

/// @addr{0x807C4DC8}
/// @brief Narrows the spatial cache of the @ref CourseColMgr to only include KCL tris defined
/// by the provided mask within a certain radius of the given position.
/// @param radius The radius within which to narrow the spatial cache
/// @param pos The position around which to narrow the spatial cache
/// @param mask The KCL flags to include in the narrowed spatial cache
void ObjColMgr::narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    CourseColMgr::Instance()->scaledNarrowScopeLocal(m_kclScale, radius, m_data, posWrtModel, mask);
}

/// @addr{0x807C4E4C}
/// @brief Computes the lower bound of the object collision bounding box in world space
/// @return The lower bound of the object collision bounding box in world space
EGG::Vector3f ObjColMgr::kclLowWorld() const {
    EGG::Vector3f posLocal = m_data->bbox().min * m_kclScale;
    return m_mtx.ps_multVector(posLocal);
}

/// @addr{0x807C4E7C}
/// @brief Computes the upper bound of the object collision bounding box in world space
/// @return The upper bound of the object collision bounding box in world space
EGG::Vector3f ObjColMgr::kclHighWorld() const {
    EGG::Vector3f posLocal = m_data->bbox().max * m_kclScale;
    return m_mtx.ps_multVector(posLocal);
}

/// @addr{0x807C4EAC}
/// @brief Checks collision between a point and course KCL tris, writing only partial collision
/// info
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    return checkPointImpl(&CourseColMgr::checkPointPartial, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x807C506C}
/// @brief Checks collision between a point and course KCL tris, writing only partial collision
/// info. Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    return checkPointImpl(&CourseColMgr::checkPointPartialPush, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x807C522C}
/// @brief Checks collision between a point and course KCL tris, writing out full collision info
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkPointImpl(&CourseColMgr::checkPointFull, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x807C53A4}
/// @brief Checks collision between a point and course KCL tris, writing out full collision
/// info. Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkPointImpl(&CourseColMgr::checkPointFullPush, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x807C551C}
/// @brief Checks collision between a sphere and course KCL tris, writing partial collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    return checkSphereImpl(&CourseColMgr::checkSpherePartial, radius, pos, prevPos, mask, info,
            maskOut);
}

/// @addr{0x807C56F8}
/// @brief Checks collision between a sphere and course KCL tris, writing partial collision
/// info. Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    return checkSphereImpl(&CourseColMgr::checkSpherePartialPush, radius, pos, prevPos, mask, info,
            maskOut);
}

/// @addr{0x807C58D4}
/// @brief Checks collision between a sphere and course KCL tris, writing full collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereFull(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereImpl(&CourseColMgr::checkSphereFull, radius, pos, prevPos, mask, info,
            maskOut);
}

/// @addr{0x807C5A68}
/// @brief Checks collision between a sphere and course KCL tris, writing full collision info.
/// Additionally pushes the collision entry into the @ref CollisionDirector cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereImpl(&CourseColMgr::checkSphereFullPush, radius, pos, prevPos, mask, info,
            maskOut);
}

/// @addr{0x807C5BFC}
/// @brief Checks collision between a point and course KCL tris by using the collision
/// director's local spatial cache, writing only partial collision info
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointCachedPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    return checkPointImpl(&CourseColMgr::checkPointCachedPartial, pos, prevPos, mask, info,
            maskOut);
}

/// @addr{0x807C5DD4}
/// @brief Checks collision between a point and course KCL tris by using the collision
/// director's local spatial cache, writing only partial collision info. Additionally pushes the
/// collision entry into the @ref CollisionDirector cache.
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointCachedPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    return checkPointImpl(&CourseColMgr::checkPointCachedPartialPush, pos, prevPos, mask, info,
            maskOut);
}

/// @addr{0x807C5FAC}
/// @brief Checks collision between a point and course KCL tris by using the collision
/// director's local spatial cache, writing out full collision info
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    return checkPointImpl(&CourseColMgr::checkPointCachedFull, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x807C613C}
/// @brief Checks collision between a point and course KCL tris by using the collision
/// director's local spatial cache, writing out full collision info. Additionally pushes the
/// collision entry into the @ref CollisionDirector cache.
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointCachedFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    return checkPointImpl(&CourseColMgr::checkPointCachedFullPush, pos, prevPos, mask, info,
            maskOut);
}

/// @addr{0x807C62CC}
/// @brief Checks collision between a sphere and course KCL tris by using the collision
/// director's local spatial cache, writing partial collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    return checkSphereImpl(&CourseColMgr::checkSphereCachedPartial, radius, pos, prevPos, mask,
            info, maskOut);
}

/// @addr{0x807C64C0}
/// @brief Checks collision between a sphere and course KCL tris by using the collision
/// director's local spatial cache, writing partial collision info. Additionally pushes the
/// collision entry into the @ref CollisionDirector cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    return checkSphereImpl(&CourseColMgr::checkSphereCachedPartialPush, radius, pos, prevPos, mask,
            info, maskOut);
}

/// @addr{0x807C66B4}
/// @brief Checks collision between a sphere and course KCL tris by using the collision
/// director's local spatial cache, writing full collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    return checkSphereImpl(&CourseColMgr::checkSphereCachedFull, radius, pos, prevPos, mask, info,
            maskOut);
}

/// @addr{0x807C6860}
/// @brief Checks collision between a sphere and course KCL tris by using the collision
/// director's local spatial cache, writing full collision info. Additionally pushes the
/// collision entry into the @ref CollisionDirector cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    return checkSphereImpl(&CourseColMgr::checkSphereCachedFullPush, radius, pos, prevPos, mask,
            info, maskOut);
}

/// @brief Sets the local-to-world transformation matrix
/// @param mtx The local-to-world transformation matrix to set
void ObjColMgr::setMtx(const EGG::Matrix34f &mtx) {
    m_mtx = mtx;
}

/// @brief Sets the world-to-local transformation matrix
/// @param mtx The world-to-local transformation matrix to set
void ObjColMgr::setInvMtx(const EGG::Matrix34f &mtx) {
    m_mtxInv = mtx;
}

/// @brief Sets the scale factor for KCL collision checks
/// @param val The scale factor to set
void ObjColMgr::setScale(f32 val) {
    m_kclScale = val;
}

/// @brief Sets the velocity of the moving object for collision checks
/// @param v The velocity vector to set
void ObjColMgr::setMovingObjVel(const EGG::Vector3f &v) {
    m_movingObjVel = v;
}

/// @brief Shared implementation for all point @ref ObjColMgr collision queries
/// @param fn The @ref CourseColMgr member function to dispatch the query to
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
/// @details Converts the point's current and previous positions from world space to local space. If
/// `info` was provided, then it will be populated with collision information if a collision is
/// detected.
template <typename T>
bool ObjColMgr::checkPointImpl(CourseColCheckPointFunc<T> fn, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        T tempInfo;
        tempInfo.reset();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if ((courseColMgr->*fn)(m_kclScale, m_data, posWrtModel, prevPosWrtModel, mask, &tempInfo,
                    maskOut)) {
            mergeInfo(*info, tempInfo);

            return true;
        }

        return false;
    }

    return (courseColMgr->*fn)(m_kclScale, m_data, posWrtModel, prevPosWrtModel, mask, info,
            maskOut);
}

/// @brief Shared implementation for all sphere @ref ObjColMgr collision queries
/// @param fn The @ref CourseColMgr member function to dispatch the query to
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
/// @details Converts the point's current and previous positions from world space to local space. If
/// `info` was provided, then it will be populated with collision information if a collision is
/// detected.
template <typename T>
bool ObjColMgr::checkSphereImpl(CourseColCheckSphereFunc<T> fn, f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info,
        KCLTypeMask *maskOut) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        T tempInfo;
        tempInfo.reset();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if ((courseColMgr->*fn)(m_kclScale, radius, m_data, posWrtModel, prevPosWrtModel, mask,
                    &tempInfo, maskOut)) {
            mergeInfo(*info, tempInfo);

            return true;
        }

        return false;
    }

    return (courseColMgr->*fn)(m_kclScale, radius, m_data, posWrtModel, prevPosWrtModel, mask, info,
            maskOut);
}

/// @brief Normalizes the transformed partial bbox corners and merges them into the caller's
/// bbox
/// @param info The caller's collision info object to merge into
/// @param tempInfo The temporary collision info object to merge from
void ObjColMgr::mergeInfo(CollisionInfoPartial &info, CollisionInfoPartial &tempInfo) const {
    info.transformInfo(tempInfo, m_mtx);
}

/// @brief Transforms the full collision info into world space and merges it into the caller's
/// info
/// @param info The caller's collision info object to merge into
/// @param tempInfo The temporary collision info object to merge from
void ObjColMgr::mergeInfo(CollisionInfo &info, CollisionInfo &tempInfo) const {
    info.transformInfo(tempInfo, m_mtx, m_movingObjVel);
}

} // namespace Kinoko::Field
