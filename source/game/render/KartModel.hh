#pragma once

#include "game/kart/KartObjectProxy.hh"

/// @brief Pertains to rendering the kart model.
namespace Kinoko::Render {

/// @brief Abstract class which represents the visual model of a vehicle, handling its rendering and
/// orientation
/// @details Included in Kinoko because some of its members affect the kart's physics state.
class KartModel : Kart::KartObjectProxy {
public:
    /// @brief Constructor
    /// @details Initializes @ref m_isLeaningLeft and @ref m_isLeaningRight to `false`. Initializes
    /// @ref m_leanWobble, @ref m_bodyLeanAngle, @ref m_bodyLeanStep, and @ref m_prevBurnoutYaw to
    /// zero. Initializes @ref m_leanAmplitude to `1.0f`.
    KartModel() {
        m_isLeaningLeft = false;
        m_isLeaningRight = false;
        m_leanWobble = 0.0f;
        m_leanAmplitude = 1.0f;
        m_bodyLeanAngle = 0.0f;
        m_bodyLeanStep = 0.0f;
        m_prevBurnoutYaw = 0.0f;
    }

    /// @brief Default virtual destructor
    ~KartModel() override = default;

    /// @addr{0x807C8758}
    /// @brief Initializes the kart model with the given player index and vehicle type
    /// @details Calls @ref initModel() with the player index and vehicle type. Also sets @ref
    /// m_prevBurnoutYaw to `0.0f`.
    void init() {
        initModel(param()->playerIdx(), isBike());
        m_prevBurnoutYaw = 0.0f;
    }

    /// @addr{0x807CB360}
    /// @brief Every frame, updates the kart model
    /// @details In Kinoko, we omit a lot of implementation since we only care about the kart's
    /// leaning behavior in @ref calcLean().
    void calc() {
        calcLean();
    }

    void calcLeaning();
    void calcLeanAngle();

    /// @addr{0x807CB530}
    /// @brief Dispatches to two helper functions to update the kart's leaning angle
    /// @details Calls @ref calcLeaning() and @ref calcLeanAngle().
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
    f32 m_prevBurnoutYaw; ///< Pitch of the vehicle during a burnout on the previous frame
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
