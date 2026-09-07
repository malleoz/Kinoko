#pragma once

#include "game/field/CourseColMgr.hh"

namespace Kinoko::Field {

/// @brief Manager for an object's KCL interactions.
/// @details Exposes collision queries which operate in local space rather than world space, in
/// order to allow for dynamically sized objects. Parses tris from an object's KCL file so that
/// collision queries can be performed against the object's KCL tris. Queries can be performed for
/// both a point and a sphere. Some queries will cache the result in the @ref CollisionDirector
/// collision entry cache. This class also stores a KCL scale factor so that object collision
/// queries can map local space collision info to world space in order to compensate for dynamically
/// sized objects (such as the Bowser's Castle geysers, represented by @ref ObjectFlamePoleFoot).
class ObjColMgr {
public:
    ObjColMgr(const void *file);
    ~ObjColMgr();

    /// @addr{0x807C4DC8}
    /// @brief Narrows the spatial cache of the @ref CourseColMgr to only include KCL tris defined
    /// by the provided mask within a certain radius of the given position.
    void narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask) {
        EGG::Vector3f posWrtModel = m_mtxInv.ps_multVector(pos);
        CourseColMgr::Instance()->scaledNarrowScopeLocal(m_kclScale, radius, m_data, posWrtModel,
                mask);
    }

    /// @addr{0x807C4E4C}
    /// @brief Computes the lower bound of the object collision bounding box in world space
    [[nodiscard]] EGG::Vector3f kclLowWorld() const {
        EGG::Vector3f posLocal = m_data->bbox().min * m_kclScale;
        return m_mtx.ps_multVector(posLocal);
    }

    /// @addr{0x807C4E7C}
    /// @brief Computes the upper bound of the object collision bounding box in world space
    [[nodiscard]] EGG::Vector3f kclHighWorld() const {
        EGG::Vector3f posLocal = m_data->bbox().max * m_kclScale;
        return m_mtx.ps_multVector(posLocal);
    }

    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *infoOut, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *pInfo, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *pInfo, KCLTypeMask *maskOut);

    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);

    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *pInfo, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *pInfo,
            KCLTypeMask *maskOut);

    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *pInfo,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *pInfo,
            KCLTypeMask *maskOut);

    /// @beginSetters
    /// @brief Sets the local-to-world transformation matrix
    void setMtx(const EGG::Matrix34f &mtx) {
        m_mtx = mtx;
    }

    /// @brief Sets the world-to-local transformation matrix
    void setInvMtx(const EGG::Matrix34f &mtx) {
        m_mtxInv = mtx;
    }

    void setScale(f32 val) {
        m_kclScale = val;
    }

    void setMovingObjVel(const EGG::Vector3f &v) {
        m_movingObjVel = v;
    }
    /// @endSetters

private:
    KColData *m_data;             ///< Pointer to parsed KCL tri data
    EGG::Matrix34f m_mtx;         ///< The local-to-world transformation matrix
    EGG::Matrix34f m_mtxInv;      ///< The world-to-local transformation matrix
    f32 m_kclScale;               ///< Scale factor for collision queries
    EGG::Vector3f m_movingObjVel; ///< The object's velocity
};

} // namespace Kinoko::Field
