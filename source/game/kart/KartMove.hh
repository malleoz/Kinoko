#pragma once

#include "game/kart/KartBoost.hh"
#include "game/kart/KartBurnout.hh"
#include "game/kart/KartReject.hh"

namespace Kinoko::Kart {

class KartHalfPipe;
class KartJump;
class KartScale;

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

    KartMove();
    ~KartMove() override;

    virtual void createSubsystems(const KartParam::Stats &stats);
    virtual void setTurnParams();
    virtual void clear();
    virtual void initOob();
    [[nodiscard]] virtual f32 leanRot() const;

    void setInitialPhysicsValues(const EGG::Vector3f &position, const EGG::Vector3f &angles);
    void setKartSpeedLimit();

    void calc();
    void calcRespawnTrigger();
    bool calcCollisions(f32 radius, f32 scale, EGG::Vector3f &pos, EGG::Vector3f &upLocal,
            const EGG::Vector3f &prevPos, Field::CollisionInfo *colInfo,
            Field::KCLTypeMask *maskOut, Field::KCLTypeMask flags) const;
    void calcHop();
    void calcRejectRoad();
    void calcTrickBoost();

    void applyForce(f32 force, const EGG::Vector3f &hitDir, bool stop);
    void activateMushroom();
    void activateZipperBoost();
    void enterCannon();
    void applyStartBoost(s16 frames);
    void activateCrush(u16 timer);
    void activateShrink();
    void triggerRespawn();
    void clearDrift();
    void clearRejectRoad();

    /// @beginSetters

    void setSpeed(f32 val);
    void setSmoothedUp(const EGG::Vector3f &v);
    void setUp(const EGG::Vector3f &v);
    void setDir(const EGG::Vector3f &v);
    void setIntVelDir(const EGG::Vector3f &v);
    void setFloorCollisionCount(u16 count);
    void setKCLWheelSpeedFactor(f32 val);
    void setKCLWheelRotFactor(f32 val);
    void setScale(const EGG::Vector3f &v);
    void setPadType(PadType type);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] DriftState driftState() const;
    [[nodiscard]] u16 mtCharge() const;
    [[nodiscard]] f32 kclSpeedFactor() const;
    [[nodiscard]] f32 kclRotFactor() const;
    [[nodiscard]] s32 getAppliedHopStickX() const;
    [[nodiscard]] f32 softSpeedLimit() const;
    [[nodiscard]] f32 speed() const;
    [[nodiscard]] f32 acceleration() const;
    [[nodiscard]] const EGG::Vector3f &scale() const;
    [[nodiscard]] f32 hardSpeedLimit() const;
    [[nodiscard]] const EGG::Vector3f &smoothedUp() const;
    [[nodiscard]] const EGG::Vector3f &up() const;
    [[nodiscard]] f32 totalScale() const;
    [[nodiscard]] f32 hitboxScale() const;
    [[nodiscard]] const EGG::Vector3f &dir() const;
    [[nodiscard]] const EGG::Vector3f &lastDir() const;
    [[nodiscard]] const EGG::Vector3f &intVelDir() const;
    [[nodiscard]] const EGG::Vector3f &smoothedForward() const;
    [[nodiscard]] f32 speedRatioCapped() const;
    [[nodiscard]] f32 speedRatio() const;
    [[nodiscard]] u16 floorCollisionCount() const;
    [[nodiscard]] s32 hopStickX() const;
    [[nodiscard]] f32 hopPosY() const;
    [[nodiscard]] s16 respawnBoostTimer() const;
    [[nodiscard]] s16 respawnPostLandTimer() const;
    [[nodiscard]] PadType &padType();
    [[nodiscard]] KartJump *jump();
    [[nodiscard]] const KartJump *jump() const;
    [[nodiscard]] KartHalfPipe *halfPipe();
    [[nodiscard]] const KartHalfPipe *halfPipe() const;
    [[nodiscard]] KartScale *kartScale();
    [[nodiscard]] const KartScale *kartScale() const;
    [[nodiscard]] KartBurnout &burnout();

    /// @endGetters

    [[nodiscard]] static f32 CalcSlerpRate(f32 scale, const EGG::Quatf &from, const EGG::Quatf &to);

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
    virtual void calcWheelie();
    virtual void calcVehicleRotation(f32 turn);
    virtual void startHop();
    virtual void onPreDrift();
    virtual void onWallCollision();
    virtual void calcMtCharge();
    [[nodiscard]] virtual f32 getWheelieSoftSpeedLimitBonus() const;

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
    void resetDriftManual();
    void calcBoost();
    void calcRampBoost();
    void calcDisableBackwardsAccel();
    void clearJumpPad();
    void clearRampBoost();
    void clearZipperBoost();
    void clearBoost();
    void clearSsmt();
    void clearOffroadInvincibility();
    void clearShock(bool resetSpeed);
    void calcSlopeAccel();
    void calcSsmt();
    void tryStartBoostPanel();
    void tryStartBoostRamp();
    void activateBoost(KartBoost::Type type, s16 frames);
    void setOffroadInvincibility(s16 timer);
    void calcOffroadInvincibility();
    void calcMushroomBoost();
    void calcCrushed();
    void calcShock();

    [[nodiscard]] bool canHop() const;
    [[nodiscard]] bool canStartDrift() const;
    [[nodiscard]] f32 calcVehicleAcceleration() const;
    [[nodiscard]] f32 calcWallCollisionSpeedFactor(f32 &walColSeverity);
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

    KartMoveBike();
    ~KartMoveBike() override;

    void createSubsystems(const KartParam::Stats &stats) override;
    void initOob() override;
    void setTurnParams() override;
    void clear() override;
    [[nodiscard]] f32 leanRot() const override;

    void cancelWheelie();

private:
    void calcVehicleRotation(f32 turn) override;
    void calcWheelie() override;
    void onPreDrift() override;
    void onWallCollision() override;
    void calcMtCharge() override;
    void reset(bool preserveScale, bool preserveFloorCount) override;
    [[nodiscard]] f32 getWheelieSoftSpeedLimitBonus() const override;

    void startWheelie();
    void calcWheelieInput();

    [[nodiscard]] bool canWheelie() const;
    [[nodiscard]] f32 wheelieRotFactor() const;

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
