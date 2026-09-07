#pragma once

#include "game/field/obj/ObjectDrivable.hh"

namespace Kinoko::Field {

/// @brief The wavy section after the first turn of Rainbow Road
/// @details Acts as a sine wave whose period is shortened over time.
class ObjectAurora final : public ObjectDrivable {
public:
    /// @addr{0x807FAB58}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectAurora(const System::MapdataGeoObj &params) : ObjectDrivable(params) {}

    /// @addr{0x807FB690}
    /// @brief Default virtual destructor
    ~ObjectAurora() override = default;

    /// @addr{0x807FB688}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x807FB684}
    /// @copybrief ObjectBase::createCollision()
    /// @details This is a no-op as collision is handled in the collision check functions.
    void createCollision() override {}

    /// @addr{0x807FB680}
    /// @copybrief ObjectBase::calcCollisionTransform()
    /// @details This is a no-op as collision is handled in the collision check functions.
    void calcCollisionTransform() override {}

    /// @addr{0x807FB5DC}
    /// @copybrief ObjectBase::getCollisionRadius()
    /// @return The fixed collision radius used for this object (`15100.0f`).
    [[nodiscard]] f32 getCollisionRadius() const override {
        return COLLISION_SIZE.z + 100.0f;
    }

    /// @addr{0x807FB59C}
    /// @copydoc ObjectDrivable::checkPointPartial()
    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override {
        return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x807FB5AC}
    /// @copydoc ObjectDrivable::checkPointPartialPush()
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override {
        return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x807FB5BC}
    /// @copydoc ObjectDrivable::checkPointFull()
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x807FB5CC}
    /// @copydoc ObjectDrivable::checkPointFullPush()
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x807FB58C}
    /// @copydoc ObjectDrivable::checkSpherePartial()
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x807FB590}
    /// @copydoc ObjectDrivable::checkSpherePartialPush()
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x807FB594}
    /// @copydoc ObjectDrivable::checkSphereFull()
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x807FB598}
    /// @copydoc ObjectDrivable::checkSphereFullPush()
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x807FB54C}
    /// @copydoc ObjectDrivable::checkPointCachedPartial()
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override {
        return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x807FB55C}
    /// @copydoc ObjectDrivable::checkPointCachedPartialPush()
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override {
        return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x807FB56C}
    /// @copydoc ObjectDrivable::checkPointCachedFull()
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x807FB57C}
    /// @copydoc ObjectDrivable::checkPointCachedFullPush()
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) override {
        return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x807FB53C}
    /// @copydoc ObjectDrivable::checkSphereCachedPartial()
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x807FB540}
    /// @copydoc ObjectDrivable::checkSphereCachedPartialPush()
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x807FB544}
    /// @copydoc ObjectDrivable::checkSphereCachedFull()
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x807FB548}
    /// @copydoc ObjectDrivable::checkSphereCachedFullPush()
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

private:
    /// @addr{0x807FB6D0}
    /// @brief Private function that checks collision between a sphere and the object, writing
    /// partial collision info
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, false);
    }

    /// @addr{0x807FB8B0}
    /// @brief Private function that checks collision between a sphere and the object, writing
    /// partial collision info
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, true);
    }

    /// @addr{0x807FBAC0}
    /// @brief Private function that checks collision between a sphere and the object, writing
    /// out full collision info
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, false);
    }

    /// @addr{0x807FBE6C}
    /// @brief Private function that checks collision between a sphere and the object, writing
    /// out full collision info. Additionally pushes the collision entry into the @ref
    /// CollisionDirector cache.
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, true);
    }

    [[nodiscard]] bool checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset, bool push);

    [[nodiscard]] bool checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset, bool push);

    [[nodiscard]] static f32 CalcRoadHeight(f32 phase, u32 t);
    [[nodiscard]] static f32 TemporalSin(f32 t);

    [[nodiscard]] bool calcCollision(f32 radius, const EGG::Vector3f &vel, u32 time,
            EGG::Vector3f &pos, EGG::Vector3f &fnrm, f32 &dist);

    /// @brief The size of the collision bounding box for the object
    static constexpr EGG::Vector3f COLLISION_SIZE = EGG::Vector3f(2000.0f, 2000.0f, 15000.0f);
};

} // namespace Kinoko::Field
