#pragma once

#include "game/field/obj/ObjectDrivable.hh"

namespace Kinoko::Field {

/// @brief The base class for a conveyor belt which induces road velocity.
class ObjectBelt : public ObjectDrivable {
public:
    ObjectBelt(const System::MapdataGeoObj &params);
    ~ObjectBelt() override;

    [[nodiscard]] LoadFlags loadFlags() const override;
    void createCollision() override;
    void calcCollisionTransform() override;
    [[nodiscard]] f32 getCollisionRadius() const override;

    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override;

    [[nodiscard]] bool checkSpherePartial(f32 /*radius*/, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override;
    [[nodiscard]] bool checkSpherePartialPush(f32 /*radius*/, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override;
    [[nodiscard]] bool checkSphereFull(f32 /*radius*/, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override;
    [[nodiscard]] bool checkSphereFullPush(f32 /*radius*/, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;

    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override;
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) override;

    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;

    /// @brief Calculates the conveyer belt's velocity at a given position based on its variant
    /// @param variant The variant of the conveyer belt
    /// @param pos The position at which to calculate the velocity
    /// @param timeOffset Optional time delta
    /// @return The velocity of the conveyer belt at the given position and variant
    [[nodiscard]] virtual EGG::Vector3f calcRoadVelocity(u32 variant, const EGG::Vector3f &pos,
            u32 timeOffset) const = 0;

    /// @brief Determines whether or not the conveyer belt is moving based on its variant
    /// @param variant The variant of the conveyer belt
    /// @param pos The position at which to check if the conveyer belt is moving
    /// @return Whether or not the conveyer belt is moving at the given position and variant
    [[nodiscard]] virtual bool isMoving(u32 variant, const EGG::Vector3f &pos) const = 0;

protected:
    f32 m_roadVel; ///< Velocity of the moving conveyer belt.

private:
    [[nodiscard]] virtual bool calcCollision(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut, u32 timeOffset);
};

} // namespace Kinoko::Field
