#pragma once

#include "game/kart/KartBoost.hh"
#include "game/kart/KartBurnout.hh"
#include "game/kart/KartHalfPipe.hh"
#include "game/kart/KartJump.hh"
#include "game/kart/KartReject.hh"
#include "game/kart/KartScale.hh"
#include "game/kart/KartState.hh"

namespace Kinoko::Kart {

/// @brief Responsible for reacting to player inputs and moving the kart
/// @details The base class is constructed for karts, whereas the derived class handles
/// bike-specific functionality and behavior.
class KartMove : protected KartObjectProxy {
public:
    /// @brief Describes the type of boost pad or ramp the kart is interacting with
    enum class ePadType {
        BoostPanel = 0, ///< A boost panel
        BoostRamp = 1,  ///< A boost panel that is trickable
        JumpPad = 2,    ///< A jump pad that bounces the player and drops the kart's speed
    };

    /// @brief Represents a bit flag for the type of pads the kart is interacting with
    typedef EGG::TBitFlag<u32, ePadType> PadType;

    /// @brief Describes the current drift state of the kart
    enum class DriftState {
        NotDrifting = 0, ///< No active drift
        ChargingMt = 1,  ///< Drifting and charging a mini-turbo
        ChargedMt = 2,   ///< Drifting and a mini-turbo has been charged
        ChargingSmt = 2, ///< Drifting, MT has been charged, and now charging a super MT
        ChargedSmt = 3,  ///< Drifting and a super mini-turbo has been charged
    };

    /// @addr{0x80577FC4}
    /// @brief Constructor
    /// @details Initializes @ref m_smoothedUp to the world up vector and @ref m_scale to the unit
    /// vector. Sets @ref m_totalScale, @ref m_hitBoxScale, @ref m_shockSpeedMultiplier, and @ref
    /// m_invScale to `1.0f`. Zeroes all bits in @ref m_padType and @ref m_flags. Sets @ref m_jump
    /// to `nullptr`.
    KartMove() {
        m_smoothedUp = EGG::Vector3f::ey;
        m_scale = EGG::Vector3f::unit;
        m_totalScale = 1.0f;
        m_hitboxScale = 1.0f;
        m_shockSpeedMultiplier = 1.0f;
        m_invScale = 1.0f;
        m_padType.makeAllZero();
        m_flags.makeAllZero();
        m_jump = nullptr;
    }

    /// @addr{0x80587B78}
    /// @brief Virtual destructor that destroys the underlying kart subsystems
    ~KartMove() override {
        EGG::egg_delete(m_jump);
        EGG::egg_delete(m_halfPipe);
        EGG::egg_delete(m_kartScale);
    }

    /// @addr{0x8057821C}
    /// @brief Constructs the underlying subsystems for the kart
    /// @param stats The stats used to initialize the @ref KartScale subsystem
    /// @details Constructs the @ref m_jump, @ref m_halfpipe, and @ref m_kartScale subsystems.
    virtual void createSubsystems(const KartParam::Stats &stats) {
        m_jump = EGG::egg_new<KartJump>(this);
        m_halfPipe = EGG::egg_new<KartHalfPipe>();
        m_kartScale = EGG::egg_new<KartScale>(stats);
    }

    virtual void setTurnParams();
    virtual void clear();

    /// @addr{0x8058974C}
    /// @brief Gets the lean rotation of the vehicle
    /// @return Always `0.0f` for karts
    [[nodiscard]] virtual f32 leanRot() const {
        return 0.0f;
    }

    void setInitialPhysicsValues(const EGG::Vector3f &position, const EGG::Vector3f &angles);
    void calc();
    void calcRespawnTrigger();
    void clearDrift();
    bool calcCollisions(f32 radius, f32 scale, EGG::Vector3f &pos, EGG::Vector3f &upLocal,
            const EGG::Vector3f &prevPos, Field::CollisionInfo *colInfo,
            Field::KCLTypeMask *maskOut, Field::KCLTypeMask flags) const;
    void applyForce(f32 force, const EGG::Vector3f &hitDir, bool stop);
    void activateMushroom();
    void activateZipperBoost();
    void enterCannon();

    /// @brief Clears the reject road state for the kart
    /// @details Clears the @ref eStatus::RejectRoadTrigger and @ref eStatus::NoSparkInvisibleWall
    /// bits from the kart's @ref Status.
    void clearRejectRoad() {
        status().resetBit(eStatus::RejectRoadTrigger, eStatus::NoSparkInvisibleWall);
    }

    /// @addr{0x80579968}
    /// @brief Calculates velocity and position induced from a drift hop
    /// @details Decays @ref m_hopVelY by 0.2% and then adds @ref m_hopGravity. Adds @ref m_hopVelY
    /// to @ref m_hopPosY. Once the kart begins falling, @ref m_hopPosY and @ref m_hopVelY are
    /// cleared so that normal gravity can take over.
    void calcHop() {
        m_hopVelY = m_hopVelY * 0.998f + m_hopGravity;
        m_hopPosY += m_hopVelY;

        if (m_hopPosY < 0.0f) {
            m_hopPosY = 0.0f;
            m_hopVelY = 0.0f;
        }
    }

    /// @addr{0x80579960}
    /// @brief Calculates reject road physics for the kart
    /// @details Dispatches to @ref KartReject::calc() to handle reject road physics
    void calcRejectRoad() {
        m_reject.calc();
    }

    /// @addr{0x80583658}
    /// @brief Clears portions of the kart's movement state when it goes out-of-bounds
    /// @details Clears states related to boosts, jump pads, ramp boosts, zipper boosts, SSMTs, and
    /// offroad invincibility.
    virtual void initOob() {
        clearBoost();
        clearJumpPad();
        clearRampBoost();
        clearZipperBoost();
        clearSsmt();
        clearOffroadInvincibility();
    }

    /// @addr{0x8058212C}
    /// @brief Activates the race start boost for a set number of frames
    /// @param frames The number of frames to apply the start boost for
    /// @details Activates the boost by calling @ref activateBoost() with the @ref
    /// KartBoost::Type::MiniTurbo type and the specified number of `frames`.
    void applyStartBoost(s16 frames) {
        activateBoost(KartBoost::Type::MiniTurbo, frames);
    }

    /// @addr{0x8057F7A8}
    /// @brief Activates a trick boost whose duration depends on vehicle type and jump pad variant
    /// @details Maps the trick @ref KartJump::m_variant to a corresponding duration using separate
    /// lookup tables for karts and bikes. Activates the boost via @ref activateBoost() with the
    /// looked-up duration.
    void calcTrickBoost() {
        static constexpr std::array<s16, 3> KART_TRICK_BOOST_DURATION = {{
                40,
                70,
                85,
        }};
        static constexpr std::array<s16, 3> BIKE_TRICK_BOOST_DURATION = {{
                45,
                80,
                95,
        }};

        if (status().onBit(eStatus::BeforeRespawn, eStatus::InAction)) {
            return;
        }

        s16 duration;
        if (isBike()) {
            duration = BIKE_TRICK_BOOST_DURATION[static_cast<u32>(m_jump->variant())];
        } else {
            duration = KART_TRICK_BOOST_DURATION[static_cast<u32>(m_jump->variant())];
        }

        activateBoost(KartBoost::Type::TrickAndZipper, duration);
    }

    /// @addr{0x80580F28}
    /// @brief Activates the crush effect for a set number of frames
    /// @param timer The number of frames to apply the crush effect for
    /// @details Sets the @ref eStatus::Crushed flag in the kart's @ref Status, sets @ref
    /// m_crushTimer to the provided `timer`, and triggers the crush animation via @ref
    /// KartScale::startCrush().
    void activateCrush(u16 timer) {
        status().setBit(eStatus::Crushed);
        m_crushTimer = timer;
        m_kartScale->startCrush();
    }

    /// @addr{0x80580768}
    /// @brief Activates the shrink effect
    /// @details Calls @ref applyShrink() with a duration of `300` frames.
    void activateShrink() {
        constexpr u16 SHRINK_DURATION = 300;

        applyShrink(SHRINK_DURATION);
    }

    /// @addr{0x805799AC}
    /// @brief Called when the screen wipes to black during a respawn
    /// @details Resets @ref m_timeInRespawn to zero and sets the @ref eStatus::TriggerRespawn flag
    /// in the kart's @ref Status.
    void triggerRespawn() {
        m_timeInRespawn = 0;
        status().setBit(eStatus::TriggerRespawn);
    }

    /// @addr{0x8057B9AC}
    /// @brief Sets the kart's hard speed limit
    /// @details Sets the kart's hard speed limit to a constant value of `120.0f`.
    void setKartSpeedLimit() {
        constexpr f32 LIMIT = 120.0f;
        m_hardSpeedLimit = LIMIT;
    }

    /// @beginSetters

    /// @brief Sets the kart's current speed
    /// @param val The new speed value to set for the kart
    void setSpeed(f32 val) {
        m_speed = val;
    }

    /// @brief Sets the kart's smoothed up vector
    /// @param v The new smoothed up vector to set for the kart
    void setSmoothedUp(const EGG::Vector3f &v) {
        m_smoothedUp = v;
    }

    /// @brief Sets the kart's up vector
    /// @param v The new up vector to set for the kart
    void setUp(const EGG::Vector3f &v) {
        m_up = v;
    }

    /// @brief Sets the kart's direction vector
    /// @param v The new direction vector to set for the kart
    void setDir(const EGG::Vector3f &v) {
        m_dir = v;
    }

    /// @brief Sets the kart's internal velocity direction vector
    /// @param v The new internal velocity direction vector to set for the kart
    void setIntVelDir(const EGG::Vector3f &v) {
        m_intVelDir = v;
    }

    /// @brief Sets the number of tires colliding with the floor
    /// @param count The number of tires colliding with the floor
    void setFloorCollisionCount(u16 count) {
        m_floorCollisionCount = count;
    }

    /// @brief Sets the KCL wheel speed factor
    /// @param val The new KCL wheel speed factor to set for the kart
    void setKCLWheelSpeedFactor(f32 val) {
        m_kclWheelSpeedFactor = val;
    }

    /// @brief Sets the KCL wheel rotation factor
    /// @param val The new KCL wheel rotation factor to set for the kart
    void setKCLWheelRotFactor(f32 val) {
        m_kclWheelRotFactor = val;
    }

    /// @addr{0x80581720}
    /// @brief Sets the kart's scale
    /// @param v The new scale vector to set for the kart
    void setScale(const EGG::Vector3f &v) {
        m_scale = v;
    }

    /// @brief Sets the type of boost pads / ramps the kart is interacting with
    /// @param type The new pad type(s) to set for the kart
    void setPadType(PadType type) {
        m_padType = type;
    }

    /// @endSetters

    /// @beginGetters

    /// @brief Gets the current drift state of the kart
    /// @return The current drift state of the kart, @ref m_driftState
    [[nodiscard]] DriftState driftState() const {
        return m_driftState;
    }

    /// @brief Gets the current mini-turbo charge of the kart
    /// @return The current mini-turbo charge of the kart, @ref m_mtCharge
    [[nodiscard]] u16 mtCharge() const {
        return m_mtCharge;
    }

    /// @brief Gets the current KCL wheel speed factor of the kart
    /// @return The current KCL wheel speed factor of the kart, @ref m_kclSpeedFactor
    [[nodiscard]] f32 kclSpeedFactor() const {
        return m_kclSpeedFactor;
    }

    /// @brief Gets the current KCL wheel rotation factor of the kart
    /// @return The current KCL wheel rotation factor of the kart, @ref m_kclRotFactor
    [[nodiscard]] f32 kclRotFactor() const {
        return m_kclRotFactor;
    }

    /// @addr{0x8057EFF8}
    /// @brief Gets the hop direction if the vehicle is traveling fast enough
    /// @return @ref m_hopStickX if traveling >= 55% of the vehicle's base speed, otherwise zero
    [[nodiscard]] s32 getAppliedHopStickX() const {
        return canStartDrift() ? m_hopStickX : 0;
    }

    /// @brief Gets the soft speed limit of the kart
    /// @return The soft speed limit of the kart, @ref m_softSpeedLimit
    [[nodiscard]] f32 softSpeedLimit() const {
        return m_softSpeedLimit;
    }

    /// @brief Gets the current speed of the kart
    /// @return The current speed of the kart, @ref m_speed
    [[nodiscard]] f32 speed() const {
        return m_speed;
    }

    /// @brief Gets the current acceleration of the kart
    /// @return The current acceleration of the kart, @ref m_acceleration
    [[nodiscard]] f32 acceleration() const {
        return m_acceleration;
    }

    /// @brief Gets the scale of the kart
    /// @return The scale of the kart, @ref m_scale
    [[nodiscard]] const EGG::Vector3f &scale() const {
        return m_scale;
    }

    /// @brief Gets the hard speed limit of the kart
    /// @return The hard speed limit of the kart, @ref m_hardSpeedLimit
    [[nodiscard]] f32 hardSpeedLimit() const {
        return m_hardSpeedLimit;
    }

    /// @brief Gets the smoothed up vector of the kart
    /// @return The smoothed up vector of the kart, @ref m_smoothedUp
    [[nodiscard]] const EGG::Vector3f &smoothedUp() const {
        return m_smoothedUp;
    }

    /// @brief Gets the up vector of the kart
    /// @return The up vector of the kart, @ref m_up
    [[nodiscard]] const EGG::Vector3f &up() const {
        return m_up;
    }

    /// @brief Gets the overall scale factor of the kart
    /// @return The overall scale factor of the kart, @ref m_totalScale
    [[nodiscard]] f32 totalScale() const {
        return m_totalScale;
    }

    /// @brief Gets the hitbox scale factor of the kart
    /// @return The hitbox scale factor of the kart, @ref m_hitboxScale
    [[nodiscard]] f32 hitboxScale() const {
        return m_hitboxScale;
    }

    /// @brief Gets the facing direction vector of the kart
    /// @return The facing direction vector of the kart, @ref m_dir
    [[nodiscard]] const EGG::Vector3f &dir() const {
        return m_dir;
    }

    /// @brief Gets the last facing direction vector of the kart
    /// @return The last facing direction vector of the kart, @ref m_lastDir
    [[nodiscard]] const EGG::Vector3f &lastDir() const {
        return m_lastDir;
    }

    /// @brief Gets the direction of the kart's internal velocity
    /// @return The direction of the kart's internal velocity, @ref m_intVelDir
    [[nodiscard]] const EGG::Vector3f &intVelDir() const {
        return m_intVelDir;
    }

    /// @brief Gets the smoothed forward direction vector of the kart
    /// @return The smoothed forward direction vector of the kart, @ref m_smoothedForward
    [[nodiscard]] const EGG::Vector3f &smoothedForward() const {
        return m_smoothedForward;
    }

    /// @brief Gets the capped speed ratio of the kart
    /// @return The capped speed ratio of the kart, @ref m_speedRatioCapped
    [[nodiscard]] f32 speedRatioCapped() const {
        return m_speedRatioCapped;
    }

    /// @brief Gets the speed ratio of the kart
    /// @return The speed ratio of the kart, @ref m_speedRatio
    [[nodiscard]] f32 speedRatio() const {
        return m_speedRatio;
    }

    /// @brief Gets the number of kart tires colliding with the floor
    /// @return The number of tires colliding with the floor, @ref m_floorCollisionCount
    [[nodiscard]] u16 floorCollisionCount() const {
        return m_floorCollisionCount;
    }

    /// @brief Gets the direction of the active drift hop
    /// @return The direction of the active drift hop, @ref m_hopStickX
    [[nodiscard]] s32 hopStickX() const {
        return m_hopStickX;
    }

    /// @brief Gets the relative vertical position of the active drift hop
    /// @return The relative vertical position of the drift hop, @ref m_hopPosY
    [[nodiscard]] f32 hopPosY() const {
        return m_hopPosY;
    }

    /// @brief Gets the remaining duration of the respawn boost
    /// @return The remaining duration of the respawn boost, @ref m_respawnBoostTimer
    [[nodiscard]] s16 respawnBoostTimer() const {
        return m_respawnBoostTimer;
    }

    /// @brief Gets the remaining duration of the respawn boost leniency timer
    /// @return The remaining duration of the respawn boost leniency timer, @ref
    /// m_respawnPostLandTimer
    [[nodiscard]] s16 respawnPostLandTimer() const {
        return m_respawnPostLandTimer;
    }

    /// @brief Gets the type of jump pad or ramp the kart is interacting with
    /// @return The type of jump pad or ramp the kart is interacting with, @ref m_padType
    [[nodiscard]] PadType &padType() {
        return m_padType;
    }

    /// @brief Gets the @ref KartJump subsystem
    /// @return The @ref KartJump subsystem instance, @ref m_jump
    [[nodiscard]] KartJump *jump() const {
        return m_jump;
    }

    /// @brief Gets the @ref KartHalfPipe subsystem
    /// @return The @ref KartHalfPipe subsystem instance, @ref m_halfPipe
    [[nodiscard]] KartHalfPipe *halfPipe() const {
        return m_halfPipe;
    }

    /// @brief Gets the @ref KartScale subsystem
    /// @return The @ref KartScale subsystem instance, @ref m_kartScale
    [[nodiscard]] KartScale *kartScale() const {
        return m_kartScale;
    }

    /// @brief Gets the @ref KartBurnout subsystem
    /// @return The @ref KartBurnout subsystem instance, @ref m_burnout
    [[nodiscard]] KartBurnout &burnout() {
        return m_burnout;
    }

    /// @endGetters

    /// @addr{0x805879A4}
    /// @brief Calculates the spherical linear interpolation factor when rotating from one
    /// orientation to another
    /// @param scale The scale factor for the rotation
    /// @param from The starting orientation
    /// @param to The target orientation
    /// @return The interpolation factor to use for slerping between the two orientations
    [[nodiscard]] static f32 CalcSlerpRate(f32 scale, const EGG::Quatf &from,
            const EGG::Quatf &to) {
        f32 acos = EGG::Mathf::acos(std::clamp(from.dot(to), -1.0f, 1.0f));
        return acos > 0.0f ? std::min(0.1f, scale / acos) : 0.1f;
    }

protected:
    /// @brief Flags used to track various states and conditions of the kart
    enum class eFlags {
        Respawned = 0,        ///< Set when Lakitu lets go of the player, cleared when landing
        DriftReset = 1,       ///< Set when a manual drift has been cancelled
        SsmtCharged = 2,      ///< Set after holding a stand-still mini-turbo for 75 frames
        LaunchBoost = 4,      ///< Set if the kart is in a zipper or ramp boost
        SsmtLeeway = 5,       ///< If set, activates SSMT when not pressing A or B.
        TrickableSurface = 6, ///< Set when driving on a trickable surface.
        WallBounce = 8,       ///< Set when our speed loss from wall collision is > 30.0f.
    };

    /// @brief Bitfield representing the various flags defined in @ref eFlags
    typedef EGG::TBitFlag<u16, eFlags> Flags;

    /// @brief The direction the player is currently driving in
    enum class DrivingDirection {
        Forwards = 0,            ///< The kart is driving forwards
        Braking = 1,             ///< The kart is currently braking
        WaitingForBackwards = 2, ///< Holding reverse but waiting on a 15 frame delay
        Backwards = 3,           ///< The kart is reversing
    };

    /// @brief Properties for a jump pad
    struct JumpPadProperties {
        f32 minSpeed; ///< The minimum travel speed of the kart when hitting the jump pad
        f32 maxSpeed; ///< The maximum travel speed of the kart when hitting the jump pad
        f32 velY;     ///< The vertical velocity applied to the kart by the jump pad
    };

    /// @brief Houses parameters that vary between the drift type (inward bike, outward bike, kart)
    struct DriftingParameters {
        f32 hopVelY;             ///< The initial upward velocity applied when starting a drift hop
        f32 stabilizationFactor; ///< Overrides the kart's rotation stabilization factor mid-hop
        f32 _8;                  ///< Unused in Kinoko
        f32 boostRotFactor;      ///< Scalar applied to @ref m_standStillBoostRot
    };

    /// @brief Houses parameters for the currently active cannon
    struct CannonParameter {
        f32 speed;          ///< Speed that the kart travels while in the cannon
        f32 height;         ///< Peak arc height of the cannon path to create a parabolic path
        f32 decelThreshold; ///< Distance from end of cannon at which deceleration begins
        f32 endSpeed; ///< Target speed the kart will decelerate towards near the end of the cannon
    };

    virtual void reset(bool preserveScale, bool preserveFloorCount);

    void calcDive();
    void calcStandstillBoostRot();

    f32 m_baseSpeed;                     ///< Base speed for the current character/vehicle stats
    f32 m_softSpeedLimit;                ///< Speed limit, clamped to @ref m_hardSpeedLimit
    f32 m_speed;                         ///< Current speed, restricted to the soft speed limit
    f32 m_lastSpeed;                     ///< The kart's speed on the last frame
    f32 m_hardSpeedLimit;                ///< Max speed cap of 120, unless you're in a bullet (140)
    f32 m_acceleration;                  ///< The kart's acceleration from player input and boosts
    f32 m_speedDragMultiplier;           ///< Decays speed after 5 frames of airtime
    EGG::Vector3f m_smoothedUp;          ///< Smoothed upward vector
    EGG::Vector3f m_up;                  ///< The up vector of the kart's orientation
    EGG::Vector3f m_landingDir;          ///< Facing direction that is snapped on landing
    EGG::Vector3f m_dir;                 ///< Current facing direction
    EGG::Vector3f m_lastDir;             ///< Signed magnitude direction from last frame's speed
    EGG::Vector3f m_intVelDir;           ///< Direction used to compute the kart's internal velocity
    EGG::Vector3f m_smoothedForward;     ///< Smoothed forward vector
    EGG::Vector3f m_dirDiff;             ///< Eases @ref m_dir toward the target forward direction
    bool m_hasLandingDir;                ///< Whether @ref m_landingDir was set this frame
    f32 m_outsideDriftAngle;             ///< The facing angle of an outward-drifting vehicle
    f32 m_landingAngle;                  ///< Angle correction that decays after landing
    EGG::Vector3f m_outsideDriftLastDir; ///< The prior frame's outward-drifting facing direction
    f32 m_speedRatioCapped;              ///< @ref m_speedRatio but capped at 1.0f.
    f32 m_speedRatio;                    ///< Ratio between current speed and @ref m_baseSpeed
    f32 m_kclSpeedFactor;                ///< Scales the kart's offroad speed [0.0f, 1.0f]
    f32 m_kclRotFactor;                  ///< Scales the kart's offroad turning radius [0.0f, 1.0f]
    f32 m_kclWheelSpeedFactor;           ///< Min speed multiplier of all wheel floor collisions
    f32 m_kclWheelRotFactor;             ///< Min rotation multiplier of all wheel floor collisions
    u16 m_floorCollisionCount;           ///< Number of tires colliding with the floor
    s32 m_hopStickX;                     ///< Direction of an active hop (-1=right, 0=none, 1=left)
    s32 m_hopFrame;                      ///< A timer that prevents subsequent hops until reset
    EGG::Vector3f m_hopUp;               ///< The up vector when beginning a drift hop
    EGG::Vector3f m_hopDir;              ///< The direction vector when beginning a drift hop
    f32 m_divingRot;                     ///< Pitcha velocity based on up/down stick inputs
    f32 m_standStillBoostRot;            ///< Pitch velocity during stand-still boost charging
    DriftState m_driftState;             ///< The current drift state of the kart
    u16 m_mtCharge;                      ///< The current MT charge [0, 270]
    u16 m_smtCharge;                     ///< The current SMT charge [0, 300]
    f32 m_outsideDriftBonus;             ///< Added to angular velocity when outside drifting.
    KartBoost m_boost;                   ///< State management for boosts
    s16 m_zipperBoostTimer;              ///< Number of frames in a boost from a zipper
    s16 m_zipperBoostDuration;           ///< Total duration of the active boost from a zipper
    KartReject m_reject;                 ///< State management for rejected road collisions
    s16 m_offroadInvincibilityTimer;     ///< Frames until the player is affected by offroad again
    s16 m_ssmtCharge;                    ///< Number of framescharging a stand-still MT (max of 75)
    s16 m_ssmtLeewayTimer;               ///< Prevents SSMT reset if accel input drops for 1 frame
    s16 m_ssmtDisableAccelTimer;         ///< Counter that tracks delay before starting to reverse
    f32 m_realTurn;                      ///< The "true" turn magnitude
    f32 m_weightedTurn;                  ///< Magnitude+direction of stick input based on kart stats
    EGG::Vector3f m_scale;               ///< Kart per-axis scale (pertains to getting crushed)
    f32 m_totalScale;                    ///< Kart overall scale factor (pertains to shocks)
    f32 m_hitboxScale;                   ///< Kart hitbox scale factor (pertains to shocks)
    f32 m_shockSpeedMultiplier;          ///< Speed modifier when shocked
    u16 m_mushroomBoostTimer;            ///< Number of frames until the mushroom boost runs out
    f32 m_invScale;                      ///< Inverse of the Z-component of @ref m_scale
    u16 m_shockTimer;                    ///< Frames remaining in the shocked state
    u16 m_crushTimer;                    ///< Frames remaining in the crushed state
    u32 m_nonZipperAirtime;              ///< Number of frames of airtime not pertaining to zippers
    f32 m_jumpPadMinSpeed;               ///< Minimum kart travel speed for the current jump pad
    f32 m_jumpPadMaxSpeed;               ///< Maximum kart travel speed for the current jump pad
    f32 m_jumpPadBoostMultiplier;        ///< Speed multiplier when activating the current jump pad
    f32 m_jumpPadSoftSpeedLimit;         ///< Speed limit when activating the current jump pad
    const JumpPadProperties *m_jumpPadProperties; ///< Pointer to the active jump pad's properties
    u16 m_rampBoostTimer;                         ///< Frames remaining for the active ramp boost
    f32 m_autoDriftAngle;                ///< Lean angle during automatic transmission drifts
    s16 m_autoDriftStartFrameCounter;    ///< Frames a drift-worthy stick input has been held
    f32 m_cannonLength;                  ///< Length from the kart's cannon entry to the cannon exit
    EGG::Vector3f m_cannonEntryPos;      ///< Position of the kart upon entering a cannon
    EGG::Vector3f m_cannonDir;           ///< The direction of the kart while in the cannon
    EGG::Vector3f m_cannonOrthog;        ///< Upward unit vector orthogonal to the cannon direction
    EGG::Vector3f m_cannonProgress;      ///< Distance traveled so far in the cannon
    f32 m_hopVelY;                       ///< Current relative velocity due to a hop
    f32 m_hopPosY;                       ///< Current relative position due to a hop
    f32 m_hopGravity;                    ///< Always main gravity (-1.3f)
    s16 m_timeInRespawn;                 ///< Number of frames after position snap from respawn
    s16 m_respawnPreLandTimer;           ///< 4 frame respawn boost leniency timer before landing
    s16 m_respawnPostLandTimer;          ///< 4 frame respawn boost leniency timer after landing
    s16 m_respawnBoostTimer;             ///< Number of frames until the respawn boost runs out
    s16 m_bumpTimer;                     ///< Cooldown after a @ref Reaction::SmallBump collision
    DrivingDirection m_drivingDirection; ///< Current state of the kart's driving direction
    s16 m_backwardsAllowCounter;         ///< Tracks the 15f delay before reversing
    PadType m_padType;                   ///< Bitfield of the pad types currently active
    Flags m_flags;                       ///< Bitfield of various movement-related kart flags
    KartJump *m_jump;                    ///< Pointer to the underlying @ref KartJump subsystem
    KartHalfPipe *m_halfPipe;            ///< Pointer to the underlying @ref KartHalfPipe subsystem
    KartScale *m_kartScale;              ///< Pointer to the underlying @ref KartScale subsystem
    KartBurnout m_burnout;               ///< Manages the state of start boost burnout
    const DriftingParameters *m_driftingParams; ///< Pointer to the kart's drift parameters
    f32 m_rawTurn;                              ///< Raw stick magnitude and direction

    /// @brief Minimum base speed percentage required to initiate a drift
    static constexpr f32 MINIMUM_DRIFT_THRESOLD = 0.55f;

    /// @brief The cannon parameters, indexed by @ref System::MapdataCannonPoint::m_parameterIdx
    static constexpr std::array<CannonParameter, 3> CANNON_PARAMETERS = {{
            {500.0f, 0.0f, 6000.0f, -1.0f},
            {500.0f, 5000.0f, 6000.0f, -1.0f},
            {120.0f, 2000.0f, 1000.0f, 45.0f},
    }};

private:
    virtual void calcTurn();

    /// @brief Handles the wheelie lifecycle and angular velocity from wheeling
    /// @details For karts, this is a no-op since they cannot wheelie
    virtual void calcWheelie() {}

    virtual void calcVehicleRotation(f32 turn);
    virtual void startHop();

    /// @brief Called when the vehicle hops or initiates a slipdrift
    /// @details For the base class, this is a no-op since karts cannot wheelie.
    virtual void onPreDrift() {}

    /// @addr{0x80570D20}
    /// @brief Called when colliding with a wall
    /// @details For the base class, this is a no-op since karts cannot wheelie.
    virtual void onWallCollision() {}

    virtual void calcMtCharge();

    /// @addr{0x8057C3C8}
    /// @brief Returns the percent speed boost from wheelies.
    /// @return `0.0f`, since karts cannot wheelie
    [[nodiscard]] virtual f32 getWheelieSoftSpeedLimitBonus() const {
        return 0.0f;
    }

    void calcInRespawn();
    void calcRespawnBoost();
    void calcTop();
    void calcAirtimeTop();
    void calcSpecialFloor();
    void calcDirs();
    void calcStickyRoad();
    void calcOffroad();
    void calcRisingWater();
    void calcSsmtCharge();
    bool calcPreDrift();
    void calcAutoDrift();
    void calcManualDrift();
    void startManualDrift();
    void releaseMt();
    void calcDrift();
    void calcRotation();
    void calcVehicleSpeed();
    void calcSpeed();
    void calcWallCollisionStart(f32 walColSeverity);
    void tryStartJumpPad();
    void tryEndJumpPad();
    void calcZipperBoost();
    void calcScale();
    void applyShrink(u16 timer);
    void calcCannon();
    void calcRotCannon(const EGG::Vector3f &forward);
    void exitCannon();

    [[nodiscard]] f32 calcVehicleAcceleration() const;
    [[nodiscard]] f32 calcWallCollisionSpeedFactor(f32 &walColSeverity);

    /// @addr{0x8057EA50}
    /// @brief Clears the kart's drift state
    /// @details Resets @ref m_hopStickX, @ref m_hopFrame, @ref m_smtCharge, and @ref m_mtCharge to
    /// zero. Resets the @ref eStatus::Hop and @ref eStatus::DriftManual bits in the kart's @ref
    /// Status. Sets @ref m_driftState to @ref DriftState::NotDrifting.
    void resetDriftManual() {
        m_hopStickX = 0;
        m_hopFrame = 0;
        status().resetBit(eStatus::Hop, eStatus::DriftManual);
        m_driftState = DriftState::NotDrifting;
        m_smtCharge = 0;
        m_mtCharge = 0;
    }

    /// @addr{0x80582694}
    /// @brief Computes the current boost state for the kart
    /// @details Calls @ref calcRampBoost to compute the current ramp boost state. If a boost is
    /// active, then sets the @ref eStatus::Accelerate bit in the kart's @ref Status, otherwise
    /// resets the @ref eStatus::Boost bit. Finally, calls @ref calcRampBoost().
    void calcBoost() {
        auto &status = KartObjectProxy::status();

        if (m_boost.calc()) {
            status.setBit(eStatus::Accelerate);
        } else {
            status.resetBit(eStatus::Boost);
        }

        calcRampBoost();
    }

    /// @addr{0x80582804}
    /// @brief Checks if the ramp boost is still active for the kart
    /// @details If the kart is not currently in a ramp boost (indicated by the @ref
    /// eStatus::RampBoost flag in the kart's @ref Status), this function early returns. Otherwise,
    /// sets the @ref eStatus::Accelerate status bit. Decrements @ref m_rampBoostTimer and clears
    /// @ref eStatus::RampBoost when the timer hits zero.
    void calcRampBoost() {
        auto &status = KartObjectProxy::status();

        if (status.offBit(eStatus::RampBoost)) {
            return;
        }

        status.setBit(eStatus::Accelerate);
        if (--m_rampBoostTimer < 1) {
            m_rampBoostTimer = 0;
            status.resetBit(eStatus::RampBoost);
        }
    }

    /// @addr{Inlined in 0x805828CC}
    /// @brief Computes the current cooldown duration between braking and reversing
    /// @details If the @ref eStatus::DisableBackwardsAccel flag is not set in the kart's @ref
    /// Status, this function early returns. Otherwise, decrements @ref m_ssmtDisableAccelTimer. If
    /// the timer hits zero or if the @ref eFlags::SsmtLeeway and @ref eStatus::Brake flags are both
    /// toggled off, then the @ref eStatus::DisableBackwardsAccel flag is cleared and @ref
    /// m_ssmtDisableAccelTimer is reset to zero.
    void calcDisableBackwardsAccel() {
        auto &status = KartObjectProxy::status();

        if (status.offBit(eStatus::DisableBackwardsAccel)) {
            return;
        }

        if (--m_ssmtDisableAccelTimer < 0 ||
                (m_flags.offBit(eFlags::SsmtLeeway) && status.offBit(eStatus::Brake))) {
            status.resetBit(eStatus::DisableBackwardsAccel);
            m_ssmtDisableAccelTimer = 0;
        }
    }

    /// @addr{0x80582DB4}
    /// @brief Clears the jump pad state for the kart
    /// @details Resets @ref m_jumpPadMinSpeed to zero and clears the @ref eStatus::JumpPad flag in
    /// the kart's @ref Status.
    void clearJumpPad() {
        m_jumpPadMinSpeed = 0.0f;
        status().resetBit(eStatus::JumpPad);
    }

    /// @addr{0x80582DD8}
    /// @brief Clears the ramp boost state for the kart
    /// @details Resets @ref m_rampBoostTimer to zero and clears the @ref eStatus::RampBoost flag in
    /// the kart's @ref Status.
    void clearRampBoost() {
        m_rampBoostTimer = 0;
        status().resetBit(eStatus::RampBoost);
    }

    /// @addr{0x80582F38}
    /// @brief Clears the zipper boost state for the kart
    /// @details Resets @ref m_zipperBoostTimer to zero and clears the @ref eStatus::ZipperBoost
    /// flag in the kart's @ref Status.
    void clearZipperBoost() {
        m_zipperBoostTimer = 0;
        status().resetBit(eStatus::ZipperBoost);
    }

    /// @addr{0x80582D94}
    /// @brief Clears the boost state for the kart
    /// @details Calls @ref KartBoost::resetActive() and clears the @ref eStatus::Boost flag in the
    /// kart's @ref Status.
    void clearBoost() {
        m_boost.resetActive();
        status().resetBit(eStatus::Boost);
    }

    /// @addr{0x80582F58}
    /// @brief Clears the standstill mini-turbo (SSMT) state for the kart
    /// @details Resets @ref m_ssmtCharge, @ref m_ssmtLeewayTimer, and @ref m_ssmtDisableAccelTimer
    /// to zero and clears the @ref eFlags::SsmtCharged and @ref eFlags::SsmtLeeway flags in the
    /// kart's @ref Status.
    void clearSsmt() {
        m_ssmtCharge = 0;
        m_ssmtLeewayTimer = 0;
        m_ssmtDisableAccelTimer = 0;
        m_flags.resetBit(eFlags::SsmtCharged, eFlags::SsmtLeeway);
    }

    /// @addr{0x80582F7C}
    /// @brief Clears the offroad invincibility state for the kart
    /// @details Resets @ref m_offroadInvincibilityTimer to zero and clears the @ref
    /// eStatus::BoostOffroadInvincibility flag in the kart's @ref Status.
    void clearOffroadInvincibility() {
        m_offroadInvincibilityTimer = 0;
        status().resetBit(eStatus::BoostOffroadInvincibility);
    }

    /// @addr{0x80580A84}
    /// @brief Deactivates the shock effect and optionally resets the speed multiplier
    /// @param resetSpeed If `true`, resets the speed multiplier to 1.0f
    /// @details Clears the @ref eStatus::Shocked flag in the kart's @ref Status. Resets @ref
    /// m_shockTimer to zero and rescales the kart via @ref KartScale::endShrink(). If `reset` speed
    /// is `true`, then resets @ref m_shockSpeedMultiplier to `1.0f`.
    void clearShock(bool resetSpeed) {
        status().resetBit(eStatus::Shocked);
        m_shockTimer = 0;
        m_kartScale->endShrink(0);

        if (resetSpeed) {
            m_shockSpeedMultiplier = 1.0f;
        }
    }

    /// @addr{0x8057B028}
    /// @brief Applies gravity-induced rolling on slopes when the kart is near stationary
    /// @details If the the magnitude of the kart's @ref m_speed is `30.0f` or greater, then this
    /// function does nothing. Otherwise, takes the difference between `1.0f` and the vertical
    /// component of @ref m_smoothedUp and doubles it. This value is then multiplied by the
    /// Y-component of the kart's backwards-facing direction, clamped to `[-0.5f, 0.5f]`. Finally,
    /// adds this value to @ref m_speed.
    void calcSlopeAccel() {
        constexpr f32 SPEED_THRESHOLD = 30.0f;

        f32 vel = 0.0f;
        f32 initialVel = 1.0f - m_smoothedUp.y;
        if (EGG::Mathf::abs(m_speed) < SPEED_THRESHOLD && m_smoothedUp.y > 0.0f &&
                initialVel > 0.0f) {
            initialVel = std::min(initialVel * 2.0f, 2.0f);
            vel += initialVel;
            vel *= std::clamp(-bodyForward().y, -0.5f, 0.5f);
        }
        m_speed += vel;
    }

    /// @addr{Inlined in 0x805788DC}
    /// @brief Calculates whether we are charging a standstill mini-turbo
    /// @details If the kart's @ref m_speed is `10.0f` or greater, the kart is in a boost, or the
    /// player is not pressing the accelerate or brake buttons, then the kart is not considered to
    /// be charging a standstill mini-turbo and the @ref eStatus::ChargingSSMT flag is cleared in
    /// the kart's @ref Status. Otherwise, sets the @ref eStatus::ChargingSSMT flag and clears the
    /// @ref eStatus::HopStart and @ref eStatus::DriftInput flags.
    void calcSsmt() {
        auto &status = KartObjectProxy::status();

        if (EGG::Mathf::abs(m_speed) >= 10.0f || status.onBit(eStatus::Boost, eStatus::RampBoost) ||
                status.offAnyBit(eStatus::Accelerate, eStatus::Brake)) {
            status.resetBit(eStatus::ChargingSSMT);
            return;
        }

        status.setBit(eStatus::ChargingSSMT).resetBit(eStatus::HopStart, eStatus::DriftInput);
    }

    /// @addr{0x8057DA18}
    /// @brief Checks whether the vehicle can perform a hop
    /// @return `true` if the kart can perform a hop, `false` otherwise.
    /// @details Returns `true` only if the player is starting a manual drift this frame (@ref
    /// eStatus::HopStart), the kart is touching the ground (@ref eStatus::TouchingGround), and the
    /// kart is not currently in an action (@ref eStatus::InAction).
    /// @note In the base game this is virtual, but it is not overriden in any derived class, so
    /// we can de-virtualize it here.
    [[nodiscard]] bool canHop() const {
        const auto &status = KartObjectProxy::status();

        if (status.offAnyBit(eStatus::HopStart, eStatus::TouchingGround)) {
            return false;
        }

        if (status.onBit(eStatus::InAction)) {
            return false;
        }

        return true;
    }

    /// @addr{0x8057EA94}
    /// @brief Checks whether the vehicle can start drifting
    /// @return `true` if the kart is driving >= 55% its base speed, `false` otherwise.
    /// @note This doesn't bytematch the base game. The base game's implementation uses the >
    /// comparison. In practice, > is only ever used once, whereas >= is used in every other
    /// scenario, so we simplify by writing this function for the more common scenario.
    bool canStartDrift() const {
        return m_speed >= MINIMUM_DRIFT_THRESOLD * m_baseSpeed;
    }

    /// @addr{Inlined at 0x80587590}
    /// @brief Tries to activate a boost panel boost
    /// @details If the kart is in a respawn or in an action (indicated by the @ref
    /// eStatus::BeforeRespawn or @ref eStatus::InAction flags in the kart's @ref Status), the
    /// function returns early and does not activate the boost panel boost. Otherwise, activates a
    /// 60 frame boost via @ref activateBoost() and gives the kart 60 frames of offroad
    /// invincibility via @ref setOffroadInvincibility().
    void tryStartBoostPanel() {
        constexpr s16 BOOST_PANEL_DURATION = 60;

        if (status().onBit(eStatus::BeforeRespawn, eStatus::InAction)) {
            return;
        }

        activateBoost(KartBoost::Type::MushroomAndBoostPanel, BOOST_PANEL_DURATION);
        setOffroadInvincibility(BOOST_PANEL_DURATION);
    }

    /// @addr{Inlined at 0x80587590}
    /// @brief Tries to start a ramp boost
    /// @details If the kart is in a respawn or in an action (indicated by the @ref
    /// eStatus::BeforeRespawn or @ref eStatus::InAction flags in the kart's @ref Status), the
    /// function returns early and does not start the ramp boost. Otherwise, sets the @ref
    /// eStatus::RampBoost flag, sets @ref m_rampBoostTimer to `60`, and gives the kart 60 frames of
    /// offroad invincibility via @ref setOffroadInvincibility().
    void tryStartBoostRamp() {
        constexpr s16 BOOST_RAMP_DURATION = 60;

        auto &status = KartObjectProxy::status();

        if (status.onBit(eStatus::BeforeRespawn, eStatus::InAction)) {
            return;
        }

        status.setBit(eStatus::RampBoost);
        m_rampBoostTimer = BOOST_RAMP_DURATION;
        setOffroadInvincibility(BOOST_RAMP_DURATION);
    }

    /// @addr{0x8057F090}
    /// @brief Attempts to activate a boost of the specified type for a set number of frames
    /// @param type The type of boost to activate
    /// @param frames The number of frames to apply the boost for
    /// @details Dispatches to @ref KartBoost::activate() to try to activate the specified boost
    /// `type` for the given number of `frames`. If successful, sets the @ref eStatus::Boost flag in
    /// the kart's @ref Status.
    void activateBoost(KartBoost::Type type, s16 frames) {
        if (m_boost.activate(type, frames)) {
            status().setBit(eStatus::Boost);
        }
    }

    /// @addr{0x805824C8}
    /// @brief Ignores offroad KCL collision for a set amount of time
    /// @param timer Number of frames to ignore offroad slowdown for
    /// @details Also sets the @ref eStatus::BoostOffroadInvincibility flag.
    void setOffroadInvincibility(s16 timer) {
        m_offroadInvincibilityTimer = std::max(m_offroadInvincibilityTimer, timer);
        status().setBit(eStatus::BoostOffroadInvincibility);
    }

    /// @addr{0x805824F0}
    /// @brief Checks a timer to see if we are still ignoring offroad slowdown
    /// @details When @ref m_offroadInvincibilityTimer expires, resets the @ref
    /// eStatus::BoostOffroadInvincibility flag in the kart's @ref Status.
    void calcOffroadInvincibility() {
        auto &status = KartObjectProxy::status();

        if (status.offBit(eStatus::BoostOffroadInvincibility)) {
            return;
        }

        if (--m_offroadInvincibilityTimer <= 0) {
            status.resetBit(eStatus::BoostOffroadInvincibility);
        }
    }

    /// @brief Checks to see if we the mushroom boost should expire
    /// @details When @ref m_mushroomBoostTimer expires, resets the @ref eStatus::MushroomBoost
    /// flag in the kart's @ref Status.
    void calcMushroomBoost() {
        auto &status = KartObjectProxy::status();
        if (status.offBit(eStatus::MushroomBoost)) {
            return;
        }

        if (--m_mushroomBoostTimer <= 0) {
            status.resetBit(eStatus::MushroomBoost);
        }
    }

    /// @addr{0x80580F9C}
    /// @brief Checks if the crush effect is still active and decrements the crush timer if so
    /// @details When @ref m_crushTimer expires, resets the @ref eStatus::Crushed flag in the kart's
    /// @ref Status and re-scales the kart via @ref KartScale::endCrush().
    void calcCrushed() {
        if (status().offBit(eStatus::Crushed)) {
            return;
        }

        if (--m_crushTimer <= 0) {
            status().resetBit(eStatus::Crushed);
            m_kartScale->endCrush();
        }
    }

    /// @addr{0x80580998}
    /// @brief Calculates the shock's speed multiplier and checks if the shock should end
    /// @details If the @ref eStatus::Shocked flag is not set in the kart's @ref Status, then @ref
    /// m_shockSpeedMultiplier increments by `0.05f` per frame until it reaches `1.0f`. Otherwise,
    /// it decrements by `0.03f` per frame until it reaches `0.7f`. When shocker, decrements @ref
    /// m_shockTimer. If the timer hits 0, then calls @ref clearShock() to deactivate the shock
    /// effect.
    void calcShock() {
        constexpr f32 SHOCK_DEC = 0.03f;
        constexpr f32 SHOCK_INC = 0.05f;
        constexpr f32 SHOCK_MIN = 0.7f;
        constexpr f32 SHOCK_MAX = 1.0f;

        auto &status = state()->status();
        if (status.onBit(eStatus::Shocked)) {
            if (--m_shockTimer <= 0) {
                clearShock(false);
            }

            m_shockSpeedMultiplier = std::max(SHOCK_MIN, m_shockSpeedMultiplier - SHOCK_DEC);
        } else {
            m_shockSpeedMultiplier = std::min(SHOCK_MAX, m_shockSpeedMultiplier + SHOCK_INC);
        }
    }
};

/// @copybrief KartMove
/// @details This derived class has specialized behavior for bikes, such as wheelies and leaning.
/// There are also additional member variables to track the bike's unique state.
class KartMoveBike final : public KartMove {
public:
    /// @brief Represents turning information which differs only between inside/outside drift
    struct TurningParameters {
        f32 leanRotExtVelFactor; ///< Scalar applied to sideways external velocity when leaning
        f32 leanRotIncRace;      ///< Lean rotation increment during the race
        f32 leanRotCapRace;      ///< Lean rotation cap during the race
        f32 driftStickXFactor;   ///< Factor applied to the stick X input during a drift
        f32 leanRotMaxDrift;     ///< Upper bound for lean rotation during a drift
        f32 leanRotMinDrift;     ///< Lower bound for lean rotation during a drift
        f32 leanRotIncCountdown; ///< Lean rotation increment during the countdown
        f32 leanRotCapCountdown; ///< Lean rotation cap during the countdown
        f32 leanRotIncSSMT;      ///< Lean rotation increment during a stand-still mini-turbo
        f32 leanRotCapSSMT;      ///< Lean rotation cap during a stand-still mini-turbo
        f32 leanRotDecayFactor;  ///< Factor by which the lean rotation decays each frame
        u16 maxWheelieFrames;    ///< Maximum number of frames a wheelie can be held
    };

    /// @addr{0x80587B30}
    /// @brief Constructor
    /// @details Initializes @ref m_leanRot to zero.
    KartMoveBike() : m_leanRot(0.0f) {}

    /// @addr{0x80589704}
    /// @brief Default virtual destructor
    ~KartMoveBike() override = default;

    /// @addr{0x805883C4}
    /// @brief Cancels a wheelie
    /// @details Resets the @ref eStatus::Wheelie flag in the kart's @ref Status. Resets @ref
    /// m_wheelieRotDec and @ref m_autoHardStickXFrames to zero.
    /// @note In the base game this is a virtual function, but since no derived class overrides
    /// it here, we can divirtualize for Kinoko.
    void cancelWheelie() {
        status().resetBit(eStatus::Wheelie);
        m_wheelieRotDec = 0.0f;
        m_autoHardStickXFrames = 0;
    }

    /// @addr{0x80587BB8}
    /// @copydoc KartMove::createSubsystems()
    /// @details For bikes, this override creates the bike-specific @ref KartJumpBike subsystem for
    /// @ref m_jump.
    void createSubsystems(const KartParam::Stats &stats) override {
        m_jump = EGG::egg_new<KartJumpBike>(this);
        m_halfPipe = EGG::egg_new<KartHalfPipe>();
        m_kartScale = EGG::egg_new<KartScale>(stats);
    }

    /// @addr{0x80588B58}
    /// @copydocs KartMove::initOob()
    /// @details Also cancels wheelies via @ref cancelWheelie().
    void initOob() override {
        KartMove::initOob();
        cancelWheelie();
    }

    void setTurnParams() override;

    /// @addr{0x80588950}
    /// @copydoc KartMove::clear()
    /// @details Also cancels wheelies via @ref cancelWheelie().
    void clear() override {
        KartMove::clear();
        cancelWheelie();
    }

    /// @beginGetters

    /// @addr{0x805896BC}
    /// @copybrief KartMove::leanRot()
    /// @return The current lean rotation of the bike, @ref m_leanRot
    [[nodiscard]] f32 leanRot() const override {
        return m_leanRot;
    }

    /// @endGetters

private:
    void calcVehicleRotation(f32 /*turn*/) override;
    void calcWheelie() override;

    /// @addr{0x80588B30}
    /// @copybrief KartMove::onPreDrift()
    /// @details If the kart is using automatic transmission mode (indicated via the @ref
    /// eStatus::AutoDrift bit in the kart's @ref Status), cancels a wheelie via @ref
    /// cancelWheelie().
    void onPreDrift() override {
        if (status().onBit(eStatus::AutoDrift)) {
            return;
        }

        cancelWheelie();
    }

    /// @addr{0x80588FC0}
    /// @copybrief KartMove::onWallCollision()
    /// @details Simply cancels wheelies via @ref cancelWheelie().
    void onWallCollision() override {
        cancelWheelie();
    }

    void calcMtCharge() override;
    void reset(bool preserveScale, bool preserveFloorCount) override;

    /// @addr{0x80588324}
    /// @copybrief KartMove::getWheelieSoftSpeedLimitBonus()
    /// @return `0.15f` if the bike is performing a wheelie, otherwise `0.0f`.
    [[nodiscard]] f32 getWheelieSoftSpeedLimitBonus() const override {
        constexpr f32 WHEELIE_SPEED_BONUS = 0.15f;
        return status().onBit(eStatus::Wheelie) ? WHEELIE_SPEED_BONUS : 0.0f;
    }

    void startWheelie();
    void calcWheelieInput();

    /// @addr{0x80588FE0}
    /// @copybrief KartMove::canWheelie()
    /// @details The kart can wheelie if it is traveling at 30% or more of its base speed stat
    /// @note This is a virtual function in the base game, but since its base class implementation
    /// is never called, we can de-virtualize in Kinoko.
    [[nodiscard]] bool canWheelie() const {
        constexpr f32 WHEELIE_THRESHOLD = 0.3f;

        return m_speedRatioCapped >= WHEELIE_THRESHOLD && m_speed >= 0.0f;
    }

    /// @addr{0x80588860}
    /// @brief Computes the damping factor to apply to the kart's yaw rotation
    /// @return `0.2f` is the kart is currently in a wheelie, otherwise `1.0f`.
    [[nodiscard]] f32 wheelieRotFactor() const {
        constexpr f32 WHEELIE_ROTATION_FACTOR = 0.2f;

        return status().onBit(eStatus::Wheelie) ? WHEELIE_ROTATION_FACTOR : 1.0f;
    }

    f32 m_leanRot;         ///< Z-axis rotation of the bike from leaning
    f32 m_leanRotCap;      ///< The maximum leaning rotation
    f32 m_leanRotInc;      ///< The incrementor for leaning rotation
    f32 m_wheelieRot;      ///< X-axis rotation from wheeling
    f32 m_maxWheelieRot;   ///< The maximum wheelie rotation
    u32 m_wheelieFrames;   ///< Tracks wheelie duration and cancels the wheelie after 180 frames
    s16 m_wheelieCooldown; ///< The number of frames before another wheelie can start
    f32 m_wheelieRotDec;   ///< The wheelie rotation decrementor, used after a wheelie has ended
    s16 m_autoHardStickXFrames; ///< If using auto transmission, cancels wheelies after 15 frames
    const TurningParameters *m_turningParams; ///< Inside/outside drifting bike turn info
};

} // namespace Kinoko::Kart
