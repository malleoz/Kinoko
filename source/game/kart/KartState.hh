#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Kart {

/// @brief Houses various flags and other variables to track the kart's current state
/// @details Most notably, this class is the direct observer of the input state, and sets the
/// appropriate flags for KartMove to act upon the input state. This class also is responsible for
/// managing calculations of the start boost duration.
class KartState : private KartObjectProxy {
public:
    KartState();
    ~KartState();

    /// @addr{0x8059455C}
    /// @brief Initializes the kart state by resetting all relevant flags and variables
    void init() {
        reset();
    }

    void reset();

    void calcInput();
    void calc();
    void resetFlags();
    void calcCollisions();
    void calcStartBoost();
    void calcHandleStartBoost();
    void handleStartBoost(size_t idx);

    /// @addr{0x805958F0}
    /// @brief Resets bitfields pertaining to ejections (reject road, half pipe zippers, etc.)
    void resetEjection() {
        m_status.resetBit(eStatus::HalfPipeRamp, eStatus::RejectRoad);
    }

    /// @beginSetters
    void setCannonPointId(u16 val) {
        m_cannonPointId = val;
    }

    void setBoostRampType(s32 val) {
        m_boostRampType = val;
    }

    void setJumpPadVariant(s32 val) {
        m_jumpPadVariant = val;
    }

    void setHalfPipeInvisibilityTimer(s16 val) {
        m_halfPipeInvisibilityTimer = val;
    }

    void setTrickableTimer(s16 val) {
        m_trickableTimer = val;
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] bool isDrifting() const {
        return m_status.onBit(eStatus::DriftManual, eStatus::DriftAuto);
    }

    [[nodiscard]] u16 cannonPointId() const {
        return m_cannonPointId;
    }

    [[nodiscard]] s32 boostRampType() const {
        return m_boostRampType;
    }

    [[nodiscard]] s32 jumpPadVariant() const {
        return m_jumpPadVariant;
    }

    [[nodiscard]] f32 stickX() const {
        return m_stickX;
    }

    [[nodiscard]] f32 stickY() const {
        return m_stickY;
    }

    [[nodiscard]] u32 airtime() const {
        return m_airtime;
    }

    [[nodiscard]] const EGG::Vector3f &up() const {
        return m_up;
    }

    [[nodiscard]] const EGG::Vector3f &softWallNrm() const {
        return m_softWallNrm;
    }

    [[nodiscard]] f32 startBoostCharge() const {
        return m_startBoostCharge;
    }

    [[nodiscard]] s16 wallBonkTimer() const {
        return m_wallBonkTimer;
    }

    [[nodiscard]] s16 trickableTimer() const {
        return m_trickableTimer;
    }

    [[nodiscard]] Status &status() {
        return m_status;
    }

    [[nodiscard]] const Status &status() const {
        return m_status;
    }
    /// @endGetters

private:
    /// @brief Represents a start boost entry, mapping a charge range to a duration in frames
    struct StartBoostEntry {
        f32 range;  ///< The minimum charge duration for this start boost entry
        s16 frames; ///< The duration of the start boost in frames for this entry
    };

    Status m_status;             ///< The current status flags for the kart
    u32 m_airtime;               ///< The amount of time the kart has been airborne, in frames
    EGG::Vector3f m_up;          ///< The up direction vector of the kart based on collision normals
    EGG::Vector3f m_softWallNrm; ///< Direction of the soft wall collision
    s32 m_hwgTimer;              ///< Frames remaining for the "Horizontal Wall Glitch" state
    u16 m_cannonPointId;         ///< ID of the @ref System::MapdataCannonPoint currently in use
    s32 m_boostRampType;         ///< Variant of boost ramp the kart is currently on, if any
    s32 m_jumpPadVariant;        ///< Variant of the jump pad the kart is currently on, if any
    s16 m_halfPipeInvisibilityTimer; ///< Frames remaining to ignore invisible wall collision
    f32 m_stickX;                    ///< One of 15 discrete stick values from [-1.0, 1.0].
    f32 m_stickY;                    ///< One of 15 discrete stick values from [-1.0, 1.0].
    f32 m_startBoostCharge; ///< 0-1 representation of start boost charge. Burnout if >0.95f.
    size_t m_startBoostIdx; ///< Used to map @ref m_startBoostCharge to a start boost duration.
    s16 m_wallBonkTimer;    ///< 2f counter that stunts your speed after hitting a wall. @rename
    s16 m_trickableTimer;   ///< 3f grace period to allow tricks when contacting a trickable surface

    /// @addr{0x808B64F8}
    /// @brief The start boost entries mapping charge ranges to durations in frames
    static constexpr std::array<StartBoostEntry, 6> START_BOOST_ENTRIES = {{
            {0.85f, 0},
            {0.88f, 10},
            {0.905f, 20},
            {0.925f, 30},
            {0.94f, 45},
            {0.95f, 70},
    }};
};

} // namespace Kinoko::Kart
