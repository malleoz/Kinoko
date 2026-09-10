#pragma once

#include "game/field/obj/ObjectKCL.hh"

namespace Kinoko::Field {

/// @brief Represents the bulldozers at the end of Toad's Factory.
/// @details Their oscillation is represented as a sin wave, with two resting periods.
class ObjectBulldozer final : public ObjectKCL {
public:
    ObjectBulldozer(const System::MapdataGeoObj &params);

    /// @addr{0x807FE5F0}
    /// @brief Default virtual destructor
    ~ObjectBulldozer() override = default;

    void calc() override;

    /// @addr{0x807FE5E8}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x807FE4FC}
    /// @copybrief ObjectKCL::colRadiusAdditionalLength()
    /// @return The amplitude of the bulldozer's oscillation.
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return static_cast<f32>(m_amplitude);
    }

    void initCollision() override;

    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override;

    /// @addr{0x807FE03C}
    /// @copydoc ObjectKCL::checkCollision()
    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereFullPush(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x807FE2CC}
    /// @copydoc ObjectKCL::checkCollisionCached()
    [[nodiscard]] bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override {
        update(timeOffset);
        calcScale(timeOffset);

        return m_objColMgr->checkSphereCachedFullPush(radius, pos, prevPos, mask, info, maskOut);
    }

    [[nodiscard]] f32 calcPosOffset(u32 t) const;

    const EGG::Vector3f m_initialPos; ///< Position of the bulldozer when the object is created
    const EGG::Vector3f m_initialRot; ///< Rotation of the bulldozer when the object is created
    const u16 m_timeOffset;           ///< Time offset of the bulldozer's sin wave, in frames
    const u16 m_periodDenom; ///< The denominator of the bulldozer's sin wave period, in frames
    const u16 m_restFrames;  ///< How long bulldozers stop moving for
    const u16 m_fullPeriod;  ///< m_periodDenom + 2 * m_restFrames
    const u16 m_amplitude;   ///< Max displacement of the bulldozer from its initial position
    const bool m_left;       ///< Flips the sin wave direction.
    const f32 m_period;      ///< The period of the bulldozer's sin wave, in frames
    const u16 m_halfPeriod;  ///< Half the period of the bulldozer's sin wave, in frames
    EGG::Matrix34f m_rtMat;  ///< Exists solely so getUpdatedMatrix can return a reference.
};

} // namespace Kinoko::Field
