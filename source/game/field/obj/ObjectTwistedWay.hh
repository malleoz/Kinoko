#pragma once

#include "game/field/obj/ObjectDrivable.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief The wavy road in Bowser's Castle
/// @details Behaves as a sine wave with a period of 120 frames and an amplitude which varies
/// depending on your distance from the x-axis center. Also handles collision with the pole in the
/// center of the twisted road.
class ObjectTwistedWay final : public ObjectDrivable {
public:
    /// @addr{0x80813BD4}
    /// @copybrief ObjectDrivable::ObjectDrivable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    ObjectTwistedWay(const System::MapdataGeoObj &params) : ObjectDrivable(params) {}

    /// @addr{0x80814918}
    /// @brief Default virtual destructor
    ~ObjectTwistedWay() override = default;

    /// @addr{0x80813C40}
    /// @copybrief ObjectBase::init()
    /// @details Sets @ref m_introTimer to 1000.
    void init() override {
        m_introTimer = 1000;
    }

    /// @addr{0x80813CFC}
    /// @copybrief ObjectBase::calc()
    /// @details If the race has not yet started, increments @ref m_introTimer. Otherwise, this
    /// function does nothing.
    void calc() override {
        if (!System::RaceManager::Instance()->isStageReached(System::RaceManager::Stage::Race)) {
            ++m_introTimer;
        }
    }

    /// @addr{0x80814910}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that the object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8081490C}
    /// @copybrief ObjectBase::createCollision()
    /// @details no-op because collision is handled entirely within the collision functions.
    void createCollision() override {}

    /// @addr{0x80814908}
    /// @copybrief ObjectBase::calcCollisionTransform()
    /// @details no-op because collision is handled entirely within the collision functions.
    void calcCollisionTransform() override {}

    /// @addr{0x808148F8}
    /// @copybrief ObjectBase::getCollisionRadius()
    /// @return The collision radius of the wavy road, calculated as `HALF_DEPTH`.
    [[nodiscard]] f32 getCollisionRadius() const override {
        return HALF_DEPTH;
    }

    /// @addr{0x808148B8}
    /// @copydoc ObjectDrivable::checkPointPartial()
    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override {
        return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x808148C8}
    /// @copydoc ObjectDrivable::checkPointPartialPush()
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override {
        return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x808148D8}
    /// @copydoc ObjectDrivable::checkPointFull()
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x808148E8}
    /// @copydoc ObjectDrivable::checkPointFullPush()
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x808148A8}
    /// @copydoc ObjectDrivable::checkSpherePartial()
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x808148AC}
    /// @copydoc ObjectDrivable::checkSpherePartialPush()
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x808148B0}
    /// @copydoc ObjectDrivable::checkSphereFull()
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x808148B4}
    /// @copydoc ObjectDrivable::checkSphereFullPush()
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x80814868}
    /// @copydoc ObjectDrivable::checkPointCachedPartial()
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override {
        return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x80814878}
    /// @copydoc ObjectDrivable::checkPointCachedPartialPush()
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override {
        return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x80814888}
    /// @copydoc ObjectDrivable::checkPointCachedFull()
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x80814898}
    /// @copydoc ObjectDrivable::checkPointCachedFullPush()
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) override {
        return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut, 0);
    }

    /// @addr{0x80814858}
    /// @copydoc ObjectDrivable::checkSphereCachedPartial()
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x8081485C}
    /// @copydoc ObjectDrivable::checkSphereCachedPartialPush()
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x80814860}
    /// @copydoc ObjectDrivable::checkSphereCachedFull()
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

    /// @addr{0x80814864}
    /// @copydoc ObjectDrivable::checkSphereCachedFullPush()
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset);
    }

private:
    /// @addr{0x80814958}
    /// @copydoc ObjectTwistedWay::checkSpherePartial()
    [[nodiscard]] bool checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, false);
    }

    /// @addr{0x80814EA8}
    /// @copydoc ObjectTwistedWay::checkSpherePartialPush()
    [[nodiscard]] bool checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, true);
    }

    /// @addr{0x80815444}
    /// @copydoc ObjectTwistedWay::checkSphereFull()
    [[nodiscard]] bool checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, false);
    }

    /// @addr{0x80815D64}
    /// @copydoc ObjectTwistedWay::checkSphereFullPush()
    [[nodiscard]] bool checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, timeOffset, true);
    }

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
            u32 timeOffset, bool push);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkWallCollision(f32 angle, f32 radius, u32 t, const EGG::Vector3f &relPos,
            T *info, KCLTypeMask *maskOut, bool push);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkFloorCollision(f32 angle, f32 radius, const EGG::Vector3f &relPos,
            T *info, KCLTypeMask *maskOut, bool push);

    [[nodiscard]] bool checkPoleCollision(f32 radius, f32 angle, const EGG::Vector3f &relPos,
            EGG::Vector3f &tangentOff, EGG::Vector3f &fnrm, f32 &dist);

    /// @addr{0x80813F40}
    /// @brief Calculates the road's twist angle at the given time `t` and position along the z-axis
    /// @param zPercent The position along the z-axis as a percentage of the road's half-length, in
    /// the range [-1.0f, 1.0f]
    /// @param t Two times the current framecount in the wavy road's oscillation period
    /// @return The road's twist angle (radians) at the given position and time, used to derive
    /// the floor/wall normals
    [[nodiscard]] static f32 CalcWaveAngle(f32 zPercent, u32 t) {
        constexpr f32 WAVINESS = 4.0f;
        constexpr f32 AMPLITUDE = 0.2f;

        f32 angle = EGG::Mathf::SinFIdx(
                (static_cast<f32>(t) * F_PI / PERIOD_LENGTH + WAVINESS * zPercent) * RAD2FIDX);

        f32 low = zPercent - 1.0f;
        f32 high = zPercent + 1.0f;

        return AMPLITUDE * (high * (high * (low * (low * angle))));
    }

    u32 m_introTimer; ///< Tracks frames currently elapsed before race starts

    static constexpr s32 PERIOD_LENGTH = 120;  ///< Framecount of full oscillation
    static constexpr f32 HALF_WIDTH = 2000.0f; ///< Width of the wavy road
    static constexpr f32 HALF_DEPTH = 7500.0f; ///< Half the length of the wavy road
};

} // namespace Kinoko::Field
