#pragma once

#include "game/field/obj/ObjectDrivable.hh"

#include "game/system/map/MapdataGeoObj.hh"

namespace Field {

class ObjectObakeManager : public ObjectDrivable {
public:
    ObjectObakeManager(const System::MapdataGeoObj &params);
    ~ObjectObakeManager() override;

    void calc() override;

    /// @addr{0x8080BE9C}
    [[nodiscard]] u32 loadFlags() const override {
        return 3;
    }

    /// @addr{0x8080BE98}
    void createCollision() override {}

    /// @addr{0x8080BE94}
    void calcCollisionTransform() override {}

    /// @addr{0x8080BE84}
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 100000.0f;
    }

    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask flags, CollisionInfoPartial *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut);

    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);

    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut);
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);

    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);

    void addBlock(const System::MapdataGeoObj &params);

private:
    [[nodiscard]] bool checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);
};

} // namespace Field
