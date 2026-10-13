#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Kart {

/// @brief Houses various flags and other variables to track the kart's current state
/// @details Most notably, this class is the direct observer of the input state, and sets the
/// appropriate flags for KartMove to act upon the input state. This class also is responsible for
/// managing calculations of the start boost duration.
class KartState : private KartObjectProxy {
public:
    /// @addr{0x805943B4}
    /// @brief Constructor that initializes all @ref m_status bits to 0, except for @ref
    /// eStatus::AutoDrift which is set if the kart is using automatic transmission. Initializes
    /// @ref m_airtime, @ref m_cannonPointIdx, and @ref m_startBoostIdx to zero.
    KartState() {
        m_status.makeAllZero();
        m_status.changeBit(inputs()->driftIsAuto(), eStatus::AutoDrift);

        m_airtime = 0;
        m_cannonPointId = 0;
        m_startBoostIdx = 0;
    }

    /// @addr{0x80595CC4}
    /// @brief Default virtual destructor
    ~KartState() override = default;

    /// @addr{0x8059455C}
    /// @brief Initializes the kart
    /// @details Simply calls @ref reset().
    void init() {
        reset();
    }

    void reset();
    void calcInput();
    void calc();
    void resetFlags();
    void processCollisions();
    void calcStartBoost();
    void calcHandleStartBoost();
    void handleStartBoost(size_t idx);

    /// @addr{0x805958F0}
    /// @brief Resets bitfields pertaining to ejections (reject road, half pipe zippers, etc.)
    /// @details Resets the @ref eStatus::HalfPipeRamp and @ref eStatus::RejectRoad flags in the
    /// kart's @ref Status.
    void resetEjection() {
        m_status.resetBit(eStatus::HalfPipeRamp, eStatus::RejectRoad);
    }

    /// @beginSetters

    /// @brief Sets the ID of the cannon the kart is currently in
    /// @param val The ID of the cannon point to set
    void setCannonPointId(u16 val) {
        m_cannonPointId = val;
    }

    /// @brief Sets the type of boost ramp the kart is currently on
    /// @param val The boost ramp type to set
    void setBoostRampType(s32 val) {
        m_boostRampType = val;
    }

    /// @brief Sets the variant of the jump pad the kart is currently on
    /// @param val The jump pad variant to set
    void setJumpPadVariant(s32 val) {
        m_jumpPadVariant = val;
    }

    /// @brief Sets the half pipe invisibility timer
    /// @param val The value to set the timer to
    void setHalfPipeInvisibilityTimer(s16 val) {
        m_halfPipeInvisibilityTimer = val;
    }

    /// @brief Sets the trickable grace period timer
    /// @param val The value to set the trickable grace period timer to
    void setTrickableTimer(s16 val) {
        m_trickableTimer = val;
    }

    /// @endSetters

    /// @beginGetters

    /// @brief Checks if the kart is currently drifting
    /// @return `true` if the kart is drifting, `false` otherwise
    /// @details The kart is drifting if either @ref eStatus::DriftManual or @ref eStatus::DriftAuto
    /// is set in the kart's @ref Status.
    [[nodiscard]] bool isDrifting() const {
        return m_status.onBit(eStatus::DriftManual, eStatus::DriftAuto);
    }

    /// @brief Retrieves the ID of the cannon the kart is currently in
    /// @return The ID of the cannon point, @ref m_cannonPointId
    [[nodiscard]] u16 cannonPointId() const {
        return m_cannonPointId;
    }

    /// @brief Gets the type of boost ramp the kart is currently on
    /// @return The boost ramp type, @ref m_boostRampType
    [[nodiscard]] s32 boostRampType() const {
        return m_boostRampType;
    }

    /// @brief Gets the variant of the jump pad the kart is currently on
    /// @return The jump pad variant, @ref m_jumpPadVariant
    [[nodiscard]] s32 jumpPadVariant() const {
        return m_jumpPadVariant;
    }

    /// @brief Gets the X-axis stick input the player is inputting
    /// @return The X-axis stick input, @ref m_stickX
    [[nodiscard]] f32 stickX() const {
        return m_stickX;
    }

    /// @brief Gets the Y-axis stick input the player is inputting
    /// @return The Y-axis stick input, @ref m_stickY
    [[nodiscard]] f32 stickY() const {
        return m_stickY;
    }

    /// @brief Gets the number of frames the kart has been airborne
    /// @return The number of frames the kart has been airborne, @ref m_airtime
    [[nodiscard]] u32 airtime() const {
        return m_airtime;
    }

    /// @brief Gets the up direction vector of the kart based on collision normals
    /// @return The up direction vector, @ref m_up
    [[nodiscard]] const EGG::Vector3f &up() const {
        return m_up;
    }

    /// @brief Gets the direction of the soft wall collision
    /// @return The soft wall normal vector, @ref m_softWallNrm
    [[nodiscard]] const EGG::Vector3f &softWallNrm() const {
        return m_softWallNrm;
    }

    /// @brief Gets the current start boost charge
    /// @return The start boost charge, @ref m_startBoostCharge
    [[nodiscard]] f32 startBoostCharge() const {
        return m_startBoostCharge;
    }

    /// @brief Gets the wall bonk timer
    /// @return The wall bonk timer, @ref m_wallBonkTimer
    [[nodiscard]] s16 wallBonkTimer() const {
        return m_wallBonkTimer;
    }

    /// @brief Gets the trickable grace period timer
    /// @return The trickable grace period timer, @ref m_trickableTimer
    [[nodiscard]] s16 trickableTimer() const {
        return m_trickableTimer;
    }

    /// @brief Gets the current status flags of the kart
    /// @return Ref to the current status, @ref m_status
    [[nodiscard]] Status &status() {
        return m_status;
    }

    /// @brief Gets the current status flags of the kart
    /// @return Const ref to the current status, @ref m_status
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
