#pragma once

#include "game/field/obj/ObjectKCL.hh"

namespace Kinoko::Field {

/// @brief Represents the bulldozers at the end of Toad's Factory.
/// @details Their oscillation is represented as a sin wave, with two resting periods.
class ObjectBulldozer final : public ObjectKCL {
public:
    ObjectBulldozer(const System::MapdataGeoObj &params);
    ~ObjectBulldozer() override;

    void calc() override;

    /// @addr{0x807FE5E8}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x807FE4FC}
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return static_cast<f32>(m_amplitude);
    }

    void initCollision() override;

    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override;

    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;

    [[nodiscard]] f32 calcPosOffset(u32 timeOffset) const;

    const EGG::Vector3f m_initialPos; ///< Position of the bulldozer when the object is created
    const EGG::Vector3f m_initialRot; ///< Rotation of the bulldozer when the object is created
    u16 m_timeOffset;                 ///< Time offset of the bulldozer's sin wave, in frames
    u16 m_periodDenom;      ///< The denominator of the bulldozer's sin wave period, in frames
    u16 m_restFrames;       ///< How long bulldozers stop moving for
    u16 m_fullPeriod;       ///< m_periodDenom + 2 * m_restFrames
    u16 m_amplitude;        ///< Max displacement of the bulldozer from its initial position
    bool m_left;            ///< Flips the sin wave direction.
    f32 m_period;           ///< The period of the bulldozer's sin wave, in frames
    u16 m_halfPeriod;       ///< Half the period of the bulldozer's sin wave, in frames
    EGG::Matrix34f m_rtMat; ///< Exists solely so getUpdatedMatrix can return a reference.
};

} // namespace Kinoko::Field
