#pragma once

#include "game/field/obj/ObjectDrivable.hh"

namespace Kinoko::Field {

/// @brief The net after the last turn of Maple Treeway
/// @details The net acts as an oscillating sine wave. Its vertical displacement varies over time
/// and along its z-axis, producing the largest displacement near the middle.
class ObjectAmi final : public ObjectDrivable {
public:
    /// @addr{0x80807ED0}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectAmi(const System::MapdataGeoObj &params) : ObjectDrivable(params) {}

    /// @addr{0x80808860}
    /// @brief Default virtual destructor
    ~ObjectAmi() override = default;

    /// @addr{0x80808858}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x80808854}
    /// @copybrief ObjectBase::createCollision()
    /// @details This is a no-op as collision is handled exclusively in @ref checkSphereImpl().
    void createCollision() override {}

    /// @addr{0x80808850}
    /// @copybrief ObjectBase::calcCollisionTransform()
    /// @details This is a no-op as collision is handled exclusively in @ref checkSphereImpl().
    void calcCollisionTransform() override {}

    /// @addr{0x80808840}
    /// @copybrief ObjectBase::getCollisionRadius()
    /// @return The fixed `15000.0f` collision radius used for this object.
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 15000.0f;
    }

    /// @addr{0x80808800}
    /// @copydoc ObjectDrivable::checkPointPartial()
    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override {
        return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x80808810}
    /// @copydoc ObjectDrivable::checkPointPartialPush()
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override {
        return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x80808820}
    /// @copydoc ObjectDrivable::checkPointFull()
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x80808830}
    /// @copydoc ObjectDrivable::checkPointFullPush()
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x808087F0}
    /// @copydoc ObjectDrivable::checkSpherePartial()
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x808087F4}
    /// @copydoc ObjectDrivable::checkSpherePartialPush()
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x808087F8}
    /// @copydoc ObjectDrivable::checkSphereFull()
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x808087FC}
    /// @copydoc ObjectDrivable::checkSphereFullPush()
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x808087B0}
    /// @copydoc ObjectDrivable::checkPointCachedPartial()
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override {
        return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x808087C0}
    /// @copydoc ObjectDrivable::checkPointCachedPartialPush()
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override {
        return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x808087D0}
    /// @copydoc ObjectDrivable::checkPointCachedFull()
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x808087E0}
    /// @copydoc ObjectDrivable::checkPointCachedFullPush()
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) override {
        return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x808087A0}
    /// @copydoc ObjectDrivable::checkSphereCachedPartial()
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x808087A4}
    /// @copydoc ObjectDrivable::checkSphereCachedPartialPush()
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x808087A8}
    /// @copydoc ObjectDrivable::checkSphereCachedFull()
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x808087AC}
    /// @copydoc ObjectDrivable::checkSphereCachedFullPush()
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

private:
    [[nodiscard]] bool checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
            u32 timeOffset, bool push);

    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &vel, u32 time,
            EGG::Vector3f &bbox, EGG::Vector3f &fnrm, f32 &dist);

    /// @addr{0x80808220}
    /// @brief Based off the provided phase and time, calculates the net surface height for use in
    /// collision checks
    [[nodiscard]] static f32 calcNetHeight(f32 phase, u32 t) {
        constexpr f32 Z_SLOPE = 910.0f;

        f32 zPhase = F_PI * (2.0f * phase) / DIMS.z;
        return SpatialSin(zPhase) * TemporalSin(t) - Z_SLOPE * zPhase;
    }

    /// @addr{0x80808578}
    /// @brief Computes a spatial sine wave as a function of the z-axis phase.
    /// @details The behavior is such that the net bounce is the most extreme when in the middle of
    /// the net and dampened as you approach the beginning or end along the z-axis.
    [[nodiscard]] static f32 SpatialSin(f32 phase) {
        return 550.0f * EGG::Mathf::SinFIdx(RAD2FIDX * (phase * 0.5f));
    }

    /// @brief Computes a sine wave as a function of time.
    /// @details This computes the up/down motion of the net, with a period of 70 frames.
    [[nodiscard]] static f32 TemporalSin(u32 t) {
        return EGG::Mathf::SinFIdx(RAD2FIDX * (F_PI * static_cast<f32>(t) / 35.0f));
    }

    /// @brief The size of the net's bounding box
    static constexpr EGG::Vector3f DIMS = EGG::Vector3f(2600.0f, 2000.0f, 13800.0f);
};

} // namespace Kinoko::Field
