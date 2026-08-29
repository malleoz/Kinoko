#include "ObjColMgr.hh"

namespace Kinoko::Field {

/// @addr{0x807C4CE8}
/// @brief Constructor that parses the KCL data from the provided file pointer
/// @param file Pointer to the .kcl file in memory
ObjColMgr::ObjColMgr(const void *file)
    : m_mtx(EGG::Matrix34f::ident), m_mtxInv(EGG::Matrix34f::ident), m_kclScale(1.0f),
      m_movingObjVel(EGG::Vector3f::zero) {
    m_data = EGG::egg_new<KColData>(file);
}

/// @addr{0x807C4D6C}
/// @brief Destructor that destroys the associated KCL data
ObjColMgr::~ObjColMgr() {
    ASSERT(m_data);
    EGG::egg_delete(m_data);
}

/// @addr{0x807C4EAC}
/// @brief Checks collision between a point and course KCL tris, writing only partial collision info
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask flags, CollisionInfoPartial *info, KCLTypeMask *typeMaskOut) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfoPartial tempInfo;
        tempInfo.bbox.setZero();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkPointPartial(m_kclScale, m_data, posWrtModel, prevPosWrtModel, flags,
                    &tempInfo, typeMaskOut)) {
            tempInfo.bbox.min = m_mtx.ps_multVector33(tempInfo.bbox.min);
            tempInfo.bbox.max = m_mtx.ps_multVector33(tempInfo.bbox.max);

            EGG::Vector3f min = tempInfo.bbox.min;
            tempInfo.bbox.min = min.minimize(tempInfo.bbox.max);
            tempInfo.bbox.max = min.maximize(tempInfo.bbox.max);

            info->bbox.min = info->bbox.min.minimize(tempInfo.bbox.min);
            info->bbox.max = info->bbox.max.maximize(tempInfo.bbox.max);

            return true;
        }

        return false;
    }

    return courseColMgr->checkPointPartial(m_kclScale, m_data, posWrtModel, prevPosWrtModel, flags,
            info, typeMaskOut);
}

/// @addr{0x807C506C}
/// @brief Checks collision between a point and course KCL tris, writing only partial collision
/// info. Additionally pushes the collision entry into the CollisionDirector's cache.
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask flags, CollisionInfoPartial *info, KCLTypeMask *typeMaskOut) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfoPartial tempInfo;
        tempInfo.bbox.setZero();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkPointPartialPush(m_kclScale, m_data, posWrtModel, prevPosWrtModel,
                    flags, &tempInfo, typeMaskOut)) {
            tempInfo.bbox.min = m_mtx.ps_multVector33(tempInfo.bbox.min);
            tempInfo.bbox.max = m_mtx.ps_multVector33(tempInfo.bbox.max);

            EGG::Vector3f min = tempInfo.bbox.min;
            tempInfo.bbox.min = min.minimize(tempInfo.bbox.max);
            tempInfo.bbox.max = min.maximize(tempInfo.bbox.max);

            info->bbox.min = info->bbox.min.minimize(tempInfo.bbox.min);
            info->bbox.max = info->bbox.max.maximize(tempInfo.bbox.max);

            return true;
        }

        return false;
    }

    return courseColMgr->checkPointPartialPush(m_kclScale, m_data, posWrtModel, prevPosWrtModel,
            flags, info, typeMaskOut);
}

/// @addr{0x807C522C}
/// @brief Checks collision between a point and course KCL tris, writing out full collision info
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask flags, CollisionInfo *info, KCLTypeMask *typeMaskOut) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfo tempInfo;
        tempInfo.reset();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkPointFull(m_kclScale, m_data, posWrtModel, prevPosWrtModel, flags,
                    &tempInfo, typeMaskOut)) {
            info->transformInfo(tempInfo, m_mtx, m_movingObjVel);

            return true;
        }

        return false;
    }

    return courseColMgr->checkPointFull(m_kclScale, m_data, posWrtModel, prevPosWrtModel, flags,
            info, typeMaskOut);
}

/// @addr{0x807C53A4}
/// @brief Checks collision between a point and course KCL tris, writing out full collision info.
/// Additionally pushes the collision entry into the CollisionDirector's cache.
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask flags, CollisionInfo *info, KCLTypeMask *typeMaskOut) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfo tempInfo;
        tempInfo.reset();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkPointFullPush(m_kclScale, m_data, posWrtModel, prevPosWrtModel,
                    flags, &tempInfo, typeMaskOut)) {
            info->transformInfo(tempInfo, m_mtx, m_movingObjVel);

            return true;
        }

        return false;
    }

    return courseColMgr->checkPointFullPush(m_kclScale, m_data, posWrtModel, prevPosWrtModel, flags,
            info, typeMaskOut);
}

/// @addr{0x807C551C}
/// @brief Checks collision between a sphere and course KCL tris, writing partial collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *info,
        KCLTypeMask *typeMaskOut) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfoPartial tempInfo;
        tempInfo.bbox.setZero();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkSpherePartial(m_kclScale, radius, m_data, posWrtModel,
                    prevPosWrtModel, flags, &tempInfo, typeMaskOut)) {
            tempInfo.bbox.min = m_mtx.ps_multVector33(tempInfo.bbox.min);
            tempInfo.bbox.max = m_mtx.ps_multVector33(tempInfo.bbox.max);

            EGG::Vector3f min = tempInfo.bbox.min;
            tempInfo.bbox.min = min.minimize(tempInfo.bbox.max);
            tempInfo.bbox.max = min.maximize(tempInfo.bbox.max);

            info->bbox.min = info->bbox.min.minimize(tempInfo.bbox.min);
            info->bbox.max = info->bbox.max.maximize(tempInfo.bbox.max);

            return true;
        }

        return false;
    }

    return courseColMgr->checkSpherePartial(m_kclScale, radius, m_data, posWrtModel,
            prevPosWrtModel, flags, info, typeMaskOut);
}

/// @addr{0x807C56F8}
/// @brief Checks collision between a sphere and course KCL tris, writing partial collision info.
/// Additionally pushes the collision entry into the CollisionDirector's cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *info,
        KCLTypeMask *typeMaskOut) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfoPartial tempInfo;
        tempInfo.bbox.setZero();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkSpherePartialPush(m_kclScale, radius, m_data, posWrtModel,
                    prevPosWrtModel, flags, &tempInfo, typeMaskOut)) {
            tempInfo.bbox.min = m_mtx.ps_multVector33(tempInfo.bbox.min);
            tempInfo.bbox.max = m_mtx.ps_multVector33(tempInfo.bbox.max);

            EGG::Vector3f min = tempInfo.bbox.min;
            tempInfo.bbox.min = min.minimize(tempInfo.bbox.max);
            tempInfo.bbox.max = min.maximize(tempInfo.bbox.max);

            info->bbox.min = info->bbox.min.minimize(tempInfo.bbox.min);
            info->bbox.max = info->bbox.max.maximize(tempInfo.bbox.max);

            return true;
        }

        return false;
    }

    return courseColMgr->checkSpherePartialPush(m_kclScale, radius, m_data, posWrtModel,
            prevPosWrtModel, flags, info, typeMaskOut);
}

/// @addr{0x807C58D4}
/// @brief Checks collision between a sphere and course KCL tris, writing full collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereFull(f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask flags, CollisionInfo *info, KCLTypeMask *typeMaskOut) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfo tempInfo;
        tempInfo.reset();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkSphereFull(m_kclScale, radius, m_data, posWrtModel, prevPosWrtModel,
                    flags, &tempInfo, typeMaskOut)) {
            info->transformInfo(tempInfo, m_mtx, m_movingObjVel);

            return true;
        }

        return false;
    }

    return courseColMgr->checkSphereFull(m_kclScale, radius, m_data, posWrtModel, prevPosWrtModel,
            flags, info, typeMaskOut);
}

/// @addr{0x807C5A68}
/// @brief Checks collision between a sphere and course KCL tris, writing full collision info.
/// Additionally pushes the collision entry into the CollisionDirector's cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfo *info,
        KCLTypeMask *typeMaskOut) {
    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfo tempInfo;
        tempInfo.reset();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkSphereFullPush(m_kclScale, radius, m_data, posWrtModel,
                    prevPosWrtModel, flags, &tempInfo, typeMaskOut)) {
            info->transformInfo(tempInfo, m_mtx, m_movingObjVel);

            return true;
        }

        return false;
    }

    return courseColMgr->checkSphereFullPush(m_kclScale, radius, m_data, posWrtModel,
            prevPosWrtModel, flags, info, typeMaskOut);
}

/// @addr{0x807C5BFC}
/// @brief Checks collision between a point and course KCL tris by using the collision director's
/// local spatial cache, writing only partial collision info
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointCachedPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask flags, CollisionInfoPartial *info, KCLTypeMask *typeMaskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfoPartial tempInfo;
        tempInfo.bbox.setZero();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkPointCachedPartial(m_kclScale, m_data, posWrtModel, prevPosWrtModel,
                    flags, &tempInfo, typeMaskOut)) {
            tempInfo.bbox.min = m_mtx.ps_multVector33(tempInfo.bbox.min);
            tempInfo.bbox.max = m_mtx.ps_multVector33(tempInfo.bbox.max);

            EGG::Vector3f min = tempInfo.bbox.min;
            tempInfo.bbox.min = min.minimize(tempInfo.bbox.max);
            tempInfo.bbox.max = min.maximize(tempInfo.bbox.max);

            info->bbox.min = info->bbox.min.minimize(tempInfo.bbox.min);
            info->bbox.max = info->bbox.max.maximize(tempInfo.bbox.max);

            return true;
        }

        return false;
    }

    return courseColMgr->checkPointCachedPartial(m_kclScale, m_data, posWrtModel, prevPosWrtModel,
            flags, info, typeMaskOut);
}

/// @addr{0x807C5DD4}
/// @brief Checks collision between a point and course KCL tris by using the collision director's
/// local spatial cache, writing only partial collision info. Additionally pushes the collision
/// entry into the CollisionDirector's cache.
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointCachedPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask flags, CollisionInfoPartial *info, KCLTypeMask *typeMaskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfoPartial tempInfo;
        tempInfo.bbox.setZero();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkPointCachedPartialPush(m_kclScale, m_data, posWrtModel,
                    prevPosWrtModel, flags, &tempInfo, typeMaskOut)) {
            tempInfo.bbox.min = m_mtx.ps_multVector33(tempInfo.bbox.min);
            tempInfo.bbox.max = m_mtx.ps_multVector33(tempInfo.bbox.max);

            EGG::Vector3f min = tempInfo.bbox.min;
            tempInfo.bbox.min = min.minimize(tempInfo.bbox.max);
            tempInfo.bbox.max = min.maximize(tempInfo.bbox.max);

            info->bbox.min = info->bbox.min.minimize(tempInfo.bbox.min);
            info->bbox.max = info->bbox.max.maximize(tempInfo.bbox.max);

            return true;
        }

        return false;
    }

    return courseColMgr->checkPointCachedPartialPush(m_kclScale, m_data, posWrtModel,
            prevPosWrtModel, flags, info, typeMaskOut);
}

/// @addr{0x807C5FAC}
/// @brief Checks collision between a point and course KCL tris by using the collision director's
/// local spatial cache, writing out full collision info
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask flags, CollisionInfo *info, KCLTypeMask *typeMaskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfo tempInfo;
        tempInfo.reset();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkPointCachedFull(m_kclScale, m_data, posWrtModel, prevPosWrtModel,
                    flags, &tempInfo, typeMaskOut)) {
            info->transformInfo(tempInfo, m_mtx, m_movingObjVel);

            return true;
        }

        return false;
    }

    return courseColMgr->checkPointCachedFull(m_kclScale, m_data, posWrtModel, prevPosWrtModel,
            flags, info, typeMaskOut);
}

/// @addr{0x807C613C}
/// @brief Checks collision between a point and course KCL tris by using the collision director's
/// local spatial cache, writing out full collision info. Additionally pushes the collision entry
/// into the CollisionDirector's cache.
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkPointCachedFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask flags, CollisionInfo *info, KCLTypeMask *typeMaskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfo tempInfo;
        tempInfo.reset();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkPointCachedFullPush(m_kclScale, m_data, posWrtModel, prevPosWrtModel,
                    flags, &tempInfo, typeMaskOut)) {
            info->transformInfo(tempInfo, m_mtx, m_movingObjVel);

            return true;
        }

        return false;
    }

    return courseColMgr->checkPointCachedFullPush(m_kclScale, m_data, posWrtModel, prevPosWrtModel,
            flags, info, typeMaskOut);
}

/// @addr{0x807C62CC}
/// @brief Checks collision between a sphere and course KCL tris by using the collision director's
/// local spatial cache, writing partial collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask typeflags, CollisionInfoPartial *info,
        KCLTypeMask *typeMaskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfoPartial tempInfo;
        tempInfo.bbox.setZero();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkSphereCachedPartial(m_kclScale, radius, m_data, posWrtModel,
                    prevPosWrtModel, typeflags, &tempInfo, typeMaskOut)) {
            tempInfo.bbox.min = m_mtx.ps_multVector33(tempInfo.bbox.min);
            tempInfo.bbox.max = m_mtx.ps_multVector33(tempInfo.bbox.max);

            EGG::Vector3f min = tempInfo.bbox.min;
            tempInfo.bbox.min = min.minimize(tempInfo.bbox.max);
            tempInfo.bbox.max = min.maximize(tempInfo.bbox.max);

            info->bbox.min = info->bbox.min.minimize(tempInfo.bbox.min);
            info->bbox.max = info->bbox.max.maximize(tempInfo.bbox.max);

            return true;
        }

        return false;
    }

    return courseColMgr->checkSphereCachedPartial(m_kclScale, radius, m_data, posWrtModel,
            prevPosWrtModel, typeflags, info, typeMaskOut);
}

/// @addr{0x807C64C0}
/// @brief Checks collision between a sphere and course KCL tris by using the collision director's
/// local spatial cache, writing partial collision info. Additionally pushes the collision entry
/// into the CollisionDirector's cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask typeflags, CollisionInfoPartial *info,
        KCLTypeMask *typeMaskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfoPartial tempInfo;
        tempInfo.bbox.setZero();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkSphereCachedPartialPush(m_kclScale, radius, m_data, posWrtModel,
                    prevPosWrtModel, typeflags, &tempInfo, typeMaskOut)) {
            tempInfo.bbox.min = m_mtx.ps_multVector33(tempInfo.bbox.min);
            tempInfo.bbox.max = m_mtx.ps_multVector33(tempInfo.bbox.max);

            EGG::Vector3f min = tempInfo.bbox.min;
            tempInfo.bbox.min = min.minimize(tempInfo.bbox.max);
            tempInfo.bbox.max = min.maximize(tempInfo.bbox.max);

            info->bbox.min = info->bbox.min.minimize(tempInfo.bbox.min);
            info->bbox.max = info->bbox.max.maximize(tempInfo.bbox.max);

            return true;
        }

        return false;
    }

    return courseColMgr->checkSphereCachedPartialPush(m_kclScale, radius, m_data, posWrtModel,
            prevPosWrtModel, typeflags, info, typeMaskOut);
}

/// @addr{0x807C66B4}
/// @brief Checks collision between a sphere and course KCL tris by using the collision director's
/// local spatial cache, writing full collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask typeflags, CollisionInfo *info,
        KCLTypeMask *typeMaskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfo tempInfo;
        tempInfo.reset();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkSphereCachedFull(m_kclScale, radius, m_data, posWrtModel,
                    prevPosWrtModel, typeflags, &tempInfo, typeMaskOut)) {
            info->transformInfo(tempInfo, m_mtx, m_movingObjVel);

            return true;
        }

        return false;
    }

    return courseColMgr->checkSphereCachedFull(m_kclScale, radius, m_data, posWrtModel,
            prevPosWrtModel, typeflags, info, typeMaskOut);
}

/// @addr{0x807C6860}
/// @brief Checks collision between a sphere and course KCL tris by using the collision director's
/// local spatial cache, writing full collision info. Additionally pushes the collision entry into
/// the CollisionDirector's cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param prevPos The previous position of the sphere, used for calculating collision depth
/// @param flags The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param typeMaskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
bool ObjColMgr::checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask typeflags, CollisionInfo *info,
        KCLTypeMask *typeMaskOut) {
    if (m_data->prismCache(0) == 0) {
        return false;
    }

    EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
    bool hasPrevY = prevPos.y != std::numeric_limits<f32>::infinity();
    EGG::Vector3f prevPosWrtModel = hasPrevY ? m_mtxInv.ps_multVector(prevPos) : EGG::Vector3f::inf;
    auto *courseColMgr = CourseColMgr::Instance();

    if (info) {
        CollisionInfo tempInfo;
        tempInfo.reset();

        if (courseColMgr->noBounceWallInfo()) {
            courseColMgr->setLocalMtx(&m_mtx);
        }

        if (courseColMgr->checkSphereCachedFullPush(m_kclScale, radius, m_data, posWrtModel,
                    prevPosWrtModel, typeflags, &tempInfo, typeMaskOut)) {
            info->transformInfo(tempInfo, m_mtx, m_movingObjVel);

            return true;
        }

        return false;
    }

    return courseColMgr->checkSphereCachedFullPush(m_kclScale, radius, m_data, posWrtModel,
            prevPosWrtModel, typeflags, info, typeMaskOut);
}

} // namespace Kinoko::Field
