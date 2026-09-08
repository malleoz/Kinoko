#pragma once

#include "game/field/obj/ObjectDrivable.hh"

namespace Kinoko::Field {

/// @brief The wavy road in Bowser's Castle
/// @details Behaves as a sine wave with a period of 120 frames and an amplitude which varies
/// depending on your distance from the x-axis center.
class ObjectTwistedWay final : public ObjectDrivable {
public:
    ObjectTwistedWay(const System::MapdataGeoObj &params);
    ~ObjectTwistedWay() override;

    /// @addr{0x80813C40}
    /// @copybrief ObjectBase::init()
    void init() override {
        m_introTimer = 1000;
    }

    void calc() override;

    /// @addr{0x80814910}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
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
            KCLTypeMask flags, CollisionInfoPartial *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSpherePartialImpl(0.0f, pos, prevPos, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x808148C8}
    /// @copydoc ObjectDrivable::checkPointPartialPush()
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask flags, CollisionInfoPartial *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSpherePartialPushImpl(0.0f, pos, prevPos, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x808148D8}
    /// @copydoc ObjectDrivable::checkPointFull()
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSphereFullImpl(0.0f, pos, prevPos, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x808148E8}
    /// @copydoc ObjectDrivable::checkPointFullPush()
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSphereFullPushImpl(0.0f, pos, prevPos, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x808148A8}
    /// @copydoc ObjectDrivable::checkSpherePartial()
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSpherePartialImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x808148AC}
    /// @copydoc ObjectDrivable::checkSpherePartialPush()
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSpherePartialPushImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut,
                timeOffset);
    }

    /// @addr{0x808148B0}
    /// @copydoc ObjectDrivable::checkSphereFull()
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereFullImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x808148B4}
    /// @copydoc ObjectDrivable::checkSphereFullPush()
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereFullPushImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x80814868}
    /// @copydoc ObjectDrivable::checkPointCachedPartial()
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut) override {
        return checkSpherePartialImpl(0.0f, pos, prevPos, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x80814878}
    /// @copydoc ObjectDrivable::checkPointCachedPartialPush()
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut) override {
        return checkSpherePartialPushImpl(0.0f, pos, prevPos, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x80814888}
    /// @copydoc ObjectDrivable::checkPointCachedFull()
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSphereFullImpl(0.0f, pos, prevPos, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x80814898}
    /// @copydoc ObjectDrivable::checkPointCachedFullPush()
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut) override {
        return checkSphereFullPushImpl(0.0f, pos, prevPos, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x80814858}
    /// @copydoc ObjectDrivable::checkSphereCachedPartial()
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSpherePartialImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x8081485C}
    /// @copydoc ObjectDrivable::checkSphereCachedPartialPush()
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSpherePartialPushImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut,
                timeOffset);
    }

    /// @addr{0x80814860}
    /// @copydoc ObjectDrivable::checkSphereCachedFull()
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereFullImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x80814864}
    /// @copydoc ObjectDrivable::checkSphereCachedFullPush()
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereFullPushImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut, timeOffset);
    }

private:
    /// @addr{0x80814958}
    [[nodiscard]] bool checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) {
        return checkSphereImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut, timeOffset, false);
    }

    /// @addr{0x80814EA8}
    [[nodiscard]] bool checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) {
        return checkSphereImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut, timeOffset, true);
    }

    /// @addr{0x80815444}
    [[nodiscard]] bool checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) {
        return checkSphereImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut, timeOffset, false);
    }

    /// @addr{0x80815D64}
    [[nodiscard]] bool checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) {
        return checkSphereImpl(radius, pos, prevPos, flags, pInfo, pFlagsOut, timeOffset, true);
    }

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, T *pInfo, KCLTypeMask *pFlagsOut,
            u32 timeOffset, bool push);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkWallCollision(f32 angle, f32 radius, u32 t, const EGG::Vector3f &relPos,
            T *pInfo, KCLTypeMask *pFlagsOut, bool push);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkFloorCollision(f32 angle, f32 radius, const EGG::Vector3f &relPos,
            T *pInfo, KCLTypeMask *pFlagsOut, bool push);

    [[nodiscard]] bool checkPoleCollision(f32 radius, f32 angle, const EGG::Vector3f &relPos,
            EGG::Vector3f &pos, EGG::Vector3f &fnrm, f32 &dist);

    [[nodiscard]] f32 calcWavePhase(f32 zPercent, u32 t);

    u32 m_introTimer; ///< Tracks frames currently elapsed before race starts

    static constexpr s32 PERIOD_LENGTH = 120;  ///< Framecount of full oscillation
    static constexpr f32 WIDTH = 2000.0f;      ///< Width of the wavy road
    static constexpr f32 HALF_DEPTH = 7500.0f; ///< Half the length of the wavy road
};

} // namespace Kinoko::Field
