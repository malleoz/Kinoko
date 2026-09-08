#pragma once

#include "game/field/obj/ObjectDrivable.hh"

namespace Kinoko::Field {

/// @brief The base class for a conveyor belt which induces road velocity.
class ObjectBelt : public ObjectDrivable {
public:
    ObjectBelt(const System::MapdataGeoObj &params);
    ~ObjectBelt() override;

    /// @addr{0x807FD79C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x807FD798}
    /// @copybrief ObjectBase::createCollision()
    void createCollision() override {}

    /// @addr{0x807FD794}
    /// @copybrief ObjectBase::calcCollisionTransform()
    /// @details no-op because collision is managed entirely by the collision functions.
    void calcCollisionTransform() override {}

    /// @addr{0x807FD784}
    /// @copybrief ObjectBase::getCollisionRadius()
    /// @return The collision radius of the conveyer belt, `25000.0f`.
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 25000.0f;
    }

    /// @addr{0x807FD750}
    /// @copybrief ObjectDrivable::checkPointPartial()
    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/,
            CollisionInfoPartial * /*info*/, KCLTypeMask * /*maskOut*/) override {
        return false;
    }

    /// @addr{0x807FD758}
    /// @copybrief ObjectDrivable::checkPointPartialPush()
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/,
            CollisionInfoPartial * /*info*/, KCLTypeMask * /*maskOut*/) override {
        return false;
    }

    /// @addr{0x807FD760}
    /// @copybrief ObjectDrivable::checkPointFull()
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/, CollisionInfo * /*info*/,
            KCLTypeMask * /*maskOut*/) override {
        return false;
    }

    /// @addr{0x807FD768}
    /// @copybrief ObjectDrivable::checkPointFullPush()
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return calcCollision(pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x807FD728}
    /// @copybrief ObjectDrivable::checkSpherePartial()
    [[nodiscard]] bool checkSpherePartial(f32 /*radius*/, const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/,
            CollisionInfoPartial * /*info*/, KCLTypeMask * /*maskOut*/,
            u32 /*timeOffset*/) override {
        return false;
    }

    /// @addr{0x807FD730}
    /// @copybrief ObjectDrivable::checkSpherePartialPush()
    [[nodiscard]] bool checkSpherePartialPush(f32 /*radius*/, const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/,
            CollisionInfoPartial * /*info*/, KCLTypeMask * /*maskOut*/,
            u32 /*timeOffset*/) override {
        return false;
    }

    /// @addr{0x807FD738}
    /// @copybrief ObjectDrivable::checkSphereFull()
    [[nodiscard]] bool checkSphereFull(f32 /*radius*/, const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/, CollisionInfo * /*info*/,
            KCLTypeMask * /*maskOut*/, u32 /*timeOffset*/) override {
        return false;
    }

    /// @addr{0x807FD740}
    /// @copybrief ObjectDrivable::checkSphereFullPush()
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereFullPush(f32 /*radius*/, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return calcCollision(pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x807FD6F4}
    /// @copybrief ObjectDrivable::checkPointCachedPartial()
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/,
            CollisionInfoPartial * /*info*/, KCLTypeMask * /*maskOut*/) override {
        return false;
    }

    /// @addr{0x807FD6FC}
    /// @copybrief ObjectDrivable::checkPointCachedPartialPush()
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/,
            CollisionInfoPartial * /*info*/, KCLTypeMask * /*maskOut*/) override {
        return false;
    }

    /// @addr{0x807FD704}
    /// @copybrief ObjectDrivable::checkPointCachedFull()
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/, CollisionInfo * /*info*/,
            KCLTypeMask * /*maskOut*/) override {
        return false;
    }

    /// @addr{0x807FD70C}
    /// @copydoc ObjectDrivable::checkPointCachedFullPush()
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) override {
        return calcCollision(pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x807FD6CC}
    /// @copybrief ObjectDrivable::checkSphereCachedPartial()
    [[nodiscard]] bool checkSphereCachedPartial(f32 /*radius*/, const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/,
            CollisionInfoPartial * /*info*/, KCLTypeMask * /*maskOut*/,
            u32 /*timeOffset*/) override {
        return false;
    }

    /// @addr{0x807FD6D4}
    /// @copybrief ObjectDrivable::checkSphereCachedPartialPush()
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 /*radius*/, const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/,
            CollisionInfoPartial * /*info*/, KCLTypeMask * /*maskOut*/,
            u32 /*timeOffset*/) override {
        return false;
    }

    /// @addr{0x807FD6DC}
    /// @copybrief ObjectDrivable::checkSphereCachedFull()
    [[nodiscard]] bool checkSphereCachedFull(f32 /*radius*/, const EGG::Vector3f & /*pos*/,
            const EGG::Vector3f & /*prevPos*/, KCLTypeMask /*mask*/, CollisionInfo * /*info*/,
            KCLTypeMask * /*maskOut*/, u32 /*timeOffset*/) override {
        return false;
    }

    /// @addr{0x807FD6E4}
    /// @copybrief ObjectDrivable::checkSphereCachedFullPush()
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedFullPush(f32 /*radius*/, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return calcCollision(pos, prevPos, mask, info, maskOut, timeOffset);
    }

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
