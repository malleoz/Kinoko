#pragma once

#include "game/kart/KartObjectProxy.hh"

/// @brief Pertains to rendering the kart model.
namespace Kinoko::Render {

/// @brief Abstract class which represents the visual model of a vehicle, handling its rendering and
/// orientation
/// @details Included in Kinoko because some of its members affect the kart's physics state.
class KartModel : Kart::KartObjectProxy {
public:
    KartModel();
    virtual ~KartModel();

    /// @addr{0x807C8758}
    /// @brief Initializes the kart model with the given player index and vehicle type
    void init() {
        initModel(param()->playerIdx(), isBike());
        m_prevBurnoutPitch = 0.0f;
    }

    /// @addr{0x807CB360}
    /// @brief Every frame, updates the kart model
    /// @details In Kinoko, we omit a lot of implementation since we only care about the kart's
    /// leaning behavior.
    void calc() {
        calcLean();
    }

    void calcLeaning();
    void calcLeanAngle();

    /// @addr{0x807CB530}
    /// @brief Dispatches to two helper functions to update the kart's leaning angle
    void calcLean() {
        calcLeaning();
        calcLeanAngle();
    }

    /// @addr{0x807C7828}
    /// @brief Initializes the kart model's internal state based on the player index and vehicle
    /// @details For Kinoko, this just initializes @ref m_isInsideDrift.
    void initModel(u8 /*playerIdx*/, bool /*isBike*/) {
        m_isInsideDrift = vehicleType() == Kart::KartParam::Stats::DriftType::Inside_Drift_Bike;
    }

private:
    bool m_isLeaningLeft;   ///< True if the kart is drifting left or leaning left during a SSMT
    bool m_isLeaningRight;  ///< True if the kart is drifting right or leaning right during a SSMT
    f32 m_leanAmplitude;    ///< A smoothed magnitude of the X stick input
    f32 m_leanWobble;       ///< Accumulated value driven by left/right X stick inputs
    f32 m_bodyLeanAngle;    ///< The lean angle of the kart's body
    f32 m_bodyLeanStep;     ///< Interpolation step to apply to @ref m_bodyLeanAngle
    bool m_isInsideDrift;   ///< Whether the kart is an inward drifting bike
    f32 m_prevBurnoutPitch; ///< Pitch of the vehicle during a burnout on the previous frame
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
    ~KartModelKart() = default;
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
    ~KartModelBike() = default;
};

} // namespace Kinoko::Render
