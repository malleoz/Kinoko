#include "ObjColMgr.hh"

namespace Kinoko::Field {

/// @brief Shared implementation for all point @ref ObjColMgr collision queries
/// @param fn The @ref CourseColMgr member function to dispatch the query to
/// @param pos The point to check
/// @param prevPos The previous position of the point, used for calculating collision depth
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
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

// Explicit instantiation, since callers of checkPointImpl()/checkSphereImpl() live in the header
// and would otherwise be unable to see these definitions.
template bool ObjColMgr::checkPointImpl<CollisionInfo>(CourseColCheckPointFunc<CollisionInfo> fn,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut);
template bool ObjColMgr::checkPointImpl<CollisionInfoPartial>(
        CourseColCheckPointFunc<CollisionInfoPartial> fn, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut);
template bool ObjColMgr::checkSphereImpl<CollisionInfo>(CourseColCheckSphereFunc<CollisionInfo> fn,
        f32 radius, const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut);
template bool ObjColMgr::checkSphereImpl<CollisionInfoPartial>(
        CourseColCheckSphereFunc<CollisionInfoPartial> fn, f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut);

} // namespace Kinoko::Field
