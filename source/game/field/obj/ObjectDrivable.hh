#pragma once

#include "game/field/obj/ObjectBase.hh"

#include "game/field/CourseColMgr.hh"
#include "game/field/KCollisionTypes.hh"
#include "game/field/ObjectCollisionBase.hh"

namespace Field {

class ObjectDrivable : public ObjectBase {
public:
    ObjectDrivable(const System::MapdataGeoObj &params);
    ~ObjectDrivable() override;

    void load() override;
    [[nodiscard]] f32 getCollisionRadius() const override;

    virtual void initCollision() {}
    virtual void loadAABB(f32 radius);

    [[nodiscard]] virtual bool checkPointPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointFull(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut) = 0;

    [[nodiscard]] virtual bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;

    virtual void narrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
            u32 timeOffset) {}

    [[nodiscard]] virtual bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointCachedFull(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut) = 0;

    [[nodiscard]] virtual bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask,
            CourseColMgr::CollisionInfoPartial *info, KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CourseColMgr::CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;
};

} // namespace Field