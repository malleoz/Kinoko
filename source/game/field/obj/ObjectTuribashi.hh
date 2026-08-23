#pragma once

#include "game/field/obj/ObjectDrivable.hh"

namespace Kinoko::Field {

/// @brief The swaying wooden bridge at the end of GCN DK Mountain
/// @details The bridge sway is calculated using a sine and cosine wave. The sine wave is used to
/// calculate the vertical displacement depending on your X and Z position. The sway is
/// amplified as you move towards the X-axis edges of the bridge, and is further maximized as you
/// approach the Z-axis center of the bridge. The cosine wave is used to create "bumps" which make
/// the bridge's planks feel more realistic as you drive along the bridge.
class ObjectTuribashi final : public ObjectDrivable {
public:
    ObjectTuribashi(const System::MapdataGeoObj &params);
    ~ObjectTuribashi() override;

    /// @addr{0x80805AD8}
    void init() override {}

    /// @addr{0x80805C24}
    void calc() override {}

    /// @addr{0x8080650C}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x80806508}
    void createCollision() override {}

    /// @addr{0x80806504}
    void calcCollisionTransform() override {}

    /// @addr{0x808064E8}
    [[nodiscard]] f32 getCollisionRadius() const override {
        return HALF_LENGTH + 100.0f;
    }

    /// @addr{0x808064A8}
    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfoPartial *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSpherePartialImpl(0.0f, v0, v1, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x808064B8}
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfoPartial *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSpherePartialPushImpl(0.0f, v0, v1, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x808064C8}
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSphereFullImpl(0.0f, v0, v1, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x808064D8}
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSphereFullPushImpl(0.0f, v0, v1, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x80806498}
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSpherePartialImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x8080649C}
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSpherePartialPushImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x808064A0}
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut,
            u32 timeOffset) override {
        return checkSphereFullImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x808064A4}
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereFullPushImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x80806458}
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfoPartial *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSpherePartialImpl(0.0f, v0, v1, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x80806468}
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfoPartial *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSpherePartialPushImpl(0.0f, v0, v1, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x80806478}
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSphereFullImpl(0.0f, v0, v1, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x80806488}
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut) override {
        return checkSphereFullPushImpl(0.0f, v0, v1, flags, pInfo, pFlagsOut, 0);
    }

    /// @addr{0x80806448}
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSpherePartialImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x8080644C}
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSpherePartialPushImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x80806450}
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereFullImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
    }

    /// @addr{0x80806454}
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) override {
        return checkSphereFullPushImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset);
    }

private:
    /// @addr{0x80806554}
    [[nodiscard]] bool checkSpherePartialImpl(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) {
        return checkSphereImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset, false);
    }

    /// @addr{0x808068A0}
    [[nodiscard]] bool checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfoPartial *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) {
        return checkSphereImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset, true);
    }

    /// @addr{0x80806C24}
    [[nodiscard]] bool checkSphereFullImpl(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) {
        return checkSphereImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset, false);
    }

    /// @addr{0x80807110}
    [[nodiscard]] bool checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset) {
        return checkSphereImpl(radius, v0, v1, flags, pInfo, pFlagsOut, timeOffset, true);
    }

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkSphereImpl(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, T *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset, bool push);

    /// Half of the bridge's width along the x-axis.
    static constexpr f32 HALF_WIDTH = 413.872f * 2.0f;

    /// Half of the bridge's length along the z-axis.
    static constexpr f32 HALF_LENGTH = 7243.3198f;
};

} // namespace Kinoko::Field
