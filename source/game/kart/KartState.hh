#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kart {

/// @brief Houses various flags and other variables to preserve the kart's state.
/// @details Most notably, this class is the direct observer of the input state,
/// and sets the appropriate flags for KartMove to act upon the input state.
/// This class also is responsible for managing calculations of the start boost duration.
/// @nosubgrouping
class KartState : KartObjectProxy {
public:
    enum class eFlag {
        Accelerate = 0, ///< Accel button is pressed.
        Brake = 1,      ///< Brake button is pressed.
        /// @brief A "fake" button, normally set if you meet the speed requirement to hop.
        /// @warning When playing back a ghost, the game will register a hop regardless of whether
        /// or not the acceleration button is pressed. This can lead to "successful" synchronization
        /// of ghosts which could not have been created legitimately in the first place.
        DriftInput = 2,
        DriftManual = 3,                ///< Currently in a drift w/ manual.
        BeforeRespawn = 4,              ///< Set on respawn collision, cleared on position snap.
        Wall3Collision = 5,             ///< Set when colliding with wall KCL #COL_TYPE_WALL_2
        WallCollision = 6,              ///< Set if we are colliding with a wall.
        HopStart = 7,                   ///< Set if @ref m_bDriftInput was toggled on this frame.
        AccelerateStart = 8,            ///< Set if @ref m_bAccelerate was toggled on this frame.
        GroundStart = 9,                ///< Set first frame landing from airtime.
        VehicleBodyFloorCollision = 10, ///< Set if the vehicle body is colliding with the floor.
        AnyWheelCollision = 11,         ///< Set when any wheel is touching floor collision.
        AllWheelsCollision = 12,        ///< Set when all wheels are touching floor collision.
        StickLeft = 13, ///< Set on left stick input. Mutually exclusive to @ref m_bStickRight.
        WallCollisionStart = 14, ///< Set if we have just started colliding with a wall.
        AirtimeOver20 = 15,      ///< Set after 20 frames of airtime, resets on landing.
        StickyRoad = 16,         ///< Like the rBC stairs
        TouchingGround = 18,     ///< Set when any part of the vehicle is colliding with floor KCL.
        Hop = 19,                ///< Set while we are in a drift hop. Clears when we land.
        Boost = 20,              ///< Set while in a boost.
        AirStart = 23,
        StickRight = 24,    ///< Set on right stick input. Mutually exclusive to @ref m_bStickLeft.
        MushroomBoost = 26, ///< Set while we are in a mushroom boost.
        SlipdriftCharge = 27, ///< Currently in a drift w/ automatic.
        DriftAuto = 28,
        Wheelie = 29, ///< Set while we are in a wheelie (even during the countdown).
        JumpPad = 30,
        RampBoost = 31,

        InAction = 32,
        TriggerRespawn = 33,
        CannonStart = 35,
        InCannon = 36,
        TrickStart = 37,
        InATrick = 38,
        BoostOffroadInvincibility = 39, ///< Set if we should ignore offroad slowdown this frame.
        HalfPipeRamp = 41,              ///< Set while colliding with zipper KCL.
        OverZipper = 42,                ///< Set while mid-air from a zipper.
        ZipperInvisibleWall = 44,       ///< Set when colliding with invisible wall above a zipper.
        ZipperBoost = 45,               ///< Set when boosting after landing from a zipper.
        ZipperStick = 46,               ///< Set while mid-air and still influenced by the zipper.
        ZipperTrick = 47,               ///< Set while tricking mid-air from a zipper.
        DisableBackwardsAccel = 48, ///< Enforces a 20f delay when reversing after charging SSMT.
        RespawnKillY = 49,          ///< Set while respawning to cap external velocity at 0.
        Burnout = 50,               ///< Set during a burnout on race start.
        TrickRot = 54,
        ChargingSSMT = 57,      ///< Tracks whether we are charging a stand-still mini-turbo.
        RejectRoad = 59,        ///< Collision which causes a change in the player's pos and rot.
        RejectRoadTrigger = 60, ///< e.g. DK Summit ending, and Maple Treeway side walls.
        Trickable = 62,

        WheelieRot = 68,
        SkipWheelCalc = 69,
        NoSparkInvisibleWall = 75,
        InRespawn = 77,
        AfterRespawn = 78,
        JumpPadDisableYsusForce = 86,

        UNK2 = 97,
        SomethingWallCollision = 99,
        SoftWallDrift = 100,
        HWG = 101, ///< Set when "Horizontal Wall Glitch" is active.
        AfterCannon = 102,
        ChargeStartBoost = 104, ///< Like @ref m_bAccelerate but during countdown.
        EndHalfPipe = 107,

        AutoDrift = 132, ///< True if auto transmission, false if manual.
    };
    typedef EGG::TBitFlagExt<8 * sizeof(u32) * 5, eFlag> Flags;

    KartState();

    void init();
    void reset();

    void calcInput();
    void calc();
    void resetFlags();
    void calcCollisions();
    void calcStartBoost();
    void calcHandleStartBoost();
    void handleStartBoost(size_t idx);
    void resetEjection();

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
        return m_flags.onBit(eFlag::DriftManual, eFlag::DriftAuto);
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

    [[nodiscard]] const EGG::Vector3f &top() const {
        return m_top;
    }

    [[nodiscard]] const EGG::Vector3f &softWallSpeed() const {
        return m_softWallSpeed;
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

    [[nodiscard]] Flags &flags() {
        return m_flags;
    }

    [[nodiscard]] const Flags &flags() const {
        return m_flags;
    }

    /// @endGetters

private:
    u32 m_airtime;
    EGG::Vector3f m_top;
    EGG::Vector3f m_softWallSpeed;
    s32 m_hwgTimer;
    u16 m_cannonPointId;
    s32 m_boostRampType;
    s32 m_jumpPadVariant;
    s16 m_halfPipeInvisibilityTimer;
    f32 m_stickX;           ///< One of 15 discrete stick values from [-1.0, 1.0].
    f32 m_stickY;           ///< One of 15 discrete stick values from [-1.0, 1.0].
    f32 m_startBoostCharge; ///< 0-1 representation of start boost charge. Burnout if >0.95f.
    size_t m_startBoostIdx; ///< Used to map @ref m_startBoostCharge to a start boost duration.
    s16 m_wallBonkTimer;    ///< 2f counter that stunts your speed after hitting a wall. @rename
    s16 m_trickableTimer;

    Flags m_flags;
};

} // namespace Kart
