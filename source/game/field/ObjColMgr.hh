#pragma once

#include "game/field/CollisionDirector.hh"
#include "game/field/CourseColMgr.hh"
#include "game/field/KColData.hh"

#include <egg/math/Matrix.hh>

namespace Field {

/// @brief Manager for an object's KCL interactions.
/// @nosubgrouping
class ObjColMgr : EGG::Disposer {
public:
    ObjColMgr(void *file);
    ~ObjColMgr() override;

    void narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask flags);

    EGG::Vector3f kclLowWorld() const;
    EGG::Vector3f kclHighWorld() const;

    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask flags, CourseColMgr::CollisionInfoPartial *infoOut,
            KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask flags, CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask flags, CourseColMgr::CollisionInfo *pInfo, KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask flags, CourseColMgr::CollisionInfo *pInfo, KCLTypeMask *typeMaskOut);

    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *typeMaskOut);

    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CourseColMgr::CollisionInfo *pInfo, KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CourseColMgr::CollisionInfo *pInfo,
            KCLTypeMask *typeMaskOut);

    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CourseColMgr::CollisionInfo *pInfo,
            KCLTypeMask *typeMaskOut);
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CourseColMgr::CollisionInfo *pInfo,
            KCLTypeMask *typeMaskOut);

    /// @beginSetters
    void setMtx(const EGG::Matrix34f &mtx);
    void setInvMtx(const EGG::Matrix34f &mtx);
    void setScale(f32 val);
    /// @endSetters

private:
    KColData *m_data;
    EGG::Matrix34f m_mtx;
    EGG::Matrix34f m_mtxInv;
    f32 m_kclScale;
};

} // namespace Field
