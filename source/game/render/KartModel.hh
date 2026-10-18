#pragma once

#include "game/kart/KartObjectProxy.hh"

/// @brief Pertains to rendering the kart model.
namespace Kinoko::Render {

/// @brief Abstract class which represents the visual model of a vehicle, handling its rendering and
/// orientation
/// @details Included in Kinoko because some of its members affect the kart's physics state.
class KartModel : private Kart::KartObjectProxy {
public:
    KartModel();
    ~KartModel() override;

    void init();
    void calc();
    void calcLeaning();
    void calcLeanAngle();
    void calcLean();
    void initModel(u8 playerIdx, bool isBike);

private:
    bool m_isLeaningLeft;  ///< True if the kart is drifting left or leaning left during a SSMT
    bool m_isLeaningRight; ///< True if the kart is drifting right or leaning right during a SSMT
    f32 m_leanAmplitude;   ///< A smoothed magnitude of the X stick input
    f32 m_leanWobble;      ///< Accumulated value driven by left/right X stick inputs
    f32 m_bodyLeanAngle;   ///< The lean angle of the kart's body
    f32 m_bodyLeanStep;    ///< Interpolation step to apply to @ref m_bodyLeanAngle
    bool m_isInsideDrift;  ///< Whether the kart is an inward drifting bike
    f32 m_prevBurnoutYaw;  ///< Pitch of the vehicle during a burnout on the previous frame
};

/// @brief Represents the visual model of a kart
/// @copydetails KartModel
class KartModelKart final : public KartModel {
public:
    /// @addr{0x807C7364}
    /// @brief Default constructor
    KartModelKart() = default;

    /// @addr{0x807CDD08}
    /// @brief Default virtual destructor
    ~KartModelKart() override = default;
};

/// @brief Represents the visual model of a bike
/// @copydetails KartModel
class KartModelBike final : public KartModel {
public:
    /// @addr{0x807CDCCC}
    /// @brief Default constructor
    KartModelBike() = default;

    /// @addr{0x807D3F58}
    /// @brief Default virtual destructor
    ~KartModelBike() override = default;
};

} // namespace Kinoko::Render
