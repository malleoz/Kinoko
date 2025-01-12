#pragma once

#include "game/field/ObjColMgr.hh"
#include "game/field/obj/ObjectDrivable.hh"

namespace Field {

class ObjectKCL : public ObjectDrivable {
public:
    ObjectKCL(const System::MapdataGeoObj &params);
    ~ObjectKCL() override;

    void calcCollisionTransform() override;
    void createCollision() override;
    [[nodiscard]] const EGG::Vector3f &getPosition() const override;
    [[nodiscard]] f32 getCollisionRadius() const override;

    void initCollision() override;
    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CourseColMgr::CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CourseColMgr::CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CourseColMgr::CollisionInfo *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CourseColMgr::CollisionInfo *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut,
            u32 timeOffset) override;
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut,
            u32 timeOffset) override;
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    void narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
            u32 timeOffset) override;
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CourseColMgr::CollisionInfo *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut,
            u32 timeOffset) override;
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut,
            u32 timeOffset) override;
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;

    virtual void update(u32 timeOffset);
    virtual void calcScale(u32 timeOffset);

    [[nodiscard]] virtual const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset);
    [[nodiscard]] virtual f32 getScaleY() const;
    [[nodiscard]] virtual f32 colRadiusAdditionalLength() const;
    [[nodiscard]] virtual bool shouldCheckColNoPush() const;
    [[nodiscard]] virtual bool shouldCheckColPush() const;
    [[nodiscard]] virtual bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] virtual bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);

private:
    ObjColMgr *m_objColMgr;
    EGG::Vector3f m_kclMidpoint;
    f32 m_bboxHalfSideLength;
    s32 m_lastMtxUpdateFrame;
    s32 m_lastScaleUpdateFrame;
};

} // namespace Field
