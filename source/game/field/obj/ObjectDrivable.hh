#pragma once

#include "game/field/CourseColMgr.hh"
#include "game/field/ObjectCollisionBase.hh"
#include "game/field/obj/ObjectBase.hh"

namespace Kinoko::Field {

class ObjectDrivable : public ObjectBase {
public:
    ObjectDrivable(const System::MapdataGeoObj &params);
    ~ObjectDrivable() override;

    void load() override;

    /// @addr{0x80682918}
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 5000.0f;
    }

    /// @brief Called once after collision creation
    virtual void initCollision() {}

    virtual void loadAABB(f32 radius);

    /// @brief Checks collision between a point and the object, writing only partial collision info
    /// @param pos The point to check
    /// @param prevPos The previous position of the point, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] virtual bool checkPointPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) = 0;

    /// @brief Checks collision between a point and the object, writing only partial collision info.
    ///        Additionally pushes the collision entry into the CollisionDirector's cache.
    /// @param pos The point to check
    /// @param prevPos The previous position of the point, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @return Whether a collision was detected
    [[nodiscard]] virtual bool checkPointPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointFull(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) = 0;

    [[nodiscard]] virtual bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;

    virtual void narrScLocal(f32 /*radius*/, const EGG::Vector3f & /*pos*/, KCLTypeMask /*mask*/,
            u32 /*timeOffset*/) {}

    [[nodiscard]] virtual bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointCachedFull(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) = 0;
    [[nodiscard]] virtual bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) = 0;

    [[nodiscard]] virtual bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;
    [[nodiscard]] virtual bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) = 0;
};

} // namespace Kinoko::Field
