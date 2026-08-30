#pragma once

#include <egg/core/BitFlag.hh>

namespace Kinoko::Kart {

/// @brief Status flags that track the current state of the kart
/// @details This is a union of the 5 bitfields contained within KartState (from offset 0x4 to 0x17)
enum class eStatus {
    /**************
     * BITFIELD 0
     *************/

    Accelerate = 0, ///< The kart is accelerating (either by controller input or boost KCL)
    Brake = 1,      ///< The brake controller button is being pressed

    /// @brief A "fake" drift button, normally set when pressing the drift input while above the
    /// speed requirement
    /// @warning When playing back a ghost, the game will register a hop regardless of whether
    /// or not the acceleration button is pressed. This means that ghosts can do a stationary hop
    /// which cannot normally be done in a live race. This can lead to "successful" synchronization
    /// of ghosts which could not have been created legitimately in the first place.
    DriftInput = 2,

    DriftManual = 3,                ///< Currently in a drift with manual transmission
    BeforeRespawn = 4,              ///< Set on respawn collision, cleared on position snap
    Wall3Collision = 5,             ///< Set when colliding with wall KCL #COL_TYPE_WALL_2
    WallCollision = 6,              ///< Colliding with KCL #COL_TYPE_WALL_ or #COL_TYPE_WALL_2
    HopStart = 7,                   ///< Starting a drift hop this frame
    AccelerateStart = 8,            ///< Starting accelerating this frame
    GroundStart = 9,                ///< First frame landing from airtime
    VehicleBodyFloorCollision = 10, ///< The vehicle body is colliding with the floor
    AnyWheelCollision = 11,         ///< Any wheel is touching floor collision
    AllWheelsCollision = 12,        ///< All wheels are touching floor collision
    StickLeft = 13,           ///< Left controller input. Mutually exclusive to @enum StickRight
    WallCollisionStart = 14,  ///< Starting colliding with a wall this frame
    AirtimeOver20 = 15,       ///< The kart has 20 frames of airtime, resets on landing
    StickyRoad = 16,          ///< Colliding with KCL #COL_TYPE_STICKY_ROAD
    TouchingGround = 18,      ///< Any part of the vehicle is colliding with floor KCL
    Hop = 19,                 ///< In a drift hop, resets on landing
    Boost = 20,               ///< Currently in a boost (MTs, mushrooms, boost panels, etc.)
    DisableAcceleration = 22, ///< The kart cannot accelerate (water on last turn of Koopa Cape)
    AirStart = 23,            ///< The kart left the ground this frame
    StickRight = 24,          ///< Right controller input. Mutually exclusive to @enum StickLeft
    LargeFlipHit = 25,        ///< The kart is in a large flip action
    MushroomBoost = 26,       ///< The kart is actively in a boost from a Mushroom
    SlipdriftBuffered = 27,   ///< Drift input is being held mid-air and drift will begin on landing
    DriftAuto = 28,           ///< Currently in a drift with automatic transmission
    Wheelie = 29,             ///< Currently in a wheelie (even during the countdown)
    JumpPad = 30,             ///< Currently mid-air due to a jump pad
    RampBoost = 31,           ///< Currently in a boost from a ramp

    /**************
     * BITFIELD 1
     *************/

    InAction = 32,       ///< Currenty in a @enum Action
    TriggerRespawn = 33, ///< The kart's position needs to be snapped to the respawn point
    CannonStart = 35,    ///< Entering a cannon after colliding with KCL #COL_TYPE_CANNON_TRIGGER
    InCannon = 36,       ///< Currently in a cannon, cleared once dropping from the cannon
    TrickStart = 37,     ///< Starting a trick this frame
    InATrick = 38,       ///< Currently performing a trick mid-air
    BoostOffroadInvincibility = 39, ///< Offroad should not slow down the kart during a boost
    HalfPipeRamp = 41,              ///< Colliding with KCL #COL_TYPE_HALFPIPE_RAMP
    OverZipper = 42,                ///< Currently mid-air from a zipper
    JumpPadMushroom = 43,           ///< Mid-air due to a mushroom bounce pad or landing
    ZipperInvisibleWall = 44,       ///< Colliding with an invisible wall above a zipper
    ZipperBoost = 45,               ///< Boosting after landing from a zipper
    ZipperStick = 46,               ///< Mid-air from a zipper and still influenced by the zipper
    ZipperTrick = 47,               ///< Tricking mid-air from a zipper
    DisableBackwardsAccel = 48,     ///< Enforces a 20f delay when reversing after charging SSMT
    RespawnKillY = 49,              ///< While respawning, caps external velocity at 0
    Burnout = 50,                   ///< The kart is in a burnout during race start
    TrickRot = 54,                  ///< Currently performing a trick rotation
    JumpPadMushroomVelYInc = 55,    ///< Gives the kart upwards velocity from a mushroom bounce pad
    ChargingSSMT = 57,              ///< Tracks whether we are charging a stand-still mini-turbo.
    RejectRoad = 59,                ///< Colliding with surfaces having the "reject road" effect
    RejectRoadTrigger = 60,         ///< The kart is being rejected by the road
    Trickable = 62,                 ///< The kart is currently able to perform a trick

    /**************
     * BITFIELD 2
     *************/

    WheelieRot = 68,             ///< The kart is pitched up due to a wheelie
    SkipWheelCalc = 69,          ///< Skips wheel collision checks for this frame
    JumpPadMushroomTrigger = 70, ///< Mid-air due to a mushroom bounce and not yet landed
    Shocked = 71,                ///< The kart is currently shrunk in size
    MovingWaterStickyRoad = 73,  ///< The kart is affected by moving water that pulls the kart down
    NoSparkInvisibleWall = 75,   ///< Colliding with any invisible wall
    CollidingOffroad = 76,       ///< Colliding with an object whose KCL induces an offroad effect
    InRespawn = 77,         ///< The kart has been placed by the respawn point and is held by Lakitu
    AfterRespawn = 78,      ///< The kart has been let go of Lakitu due to a respawn and is mid-air
    Crushed = 80,           ///< The kart is currently crushed by an object
    JumpPadFixedSpeed = 84, ///< The kart's speed is fixed due to a jump pad effect
    MovingWaterDecaySpeed = 85,   ///< Colliding with moving water causing the kart's speed to decay
    JumpPadDisableYsusForce = 86, ///< Skips applying linear force to wheels' vertical suspension
    ZipperBypassInvisWall = 87,   ///< Bypassing KCL #COL_TYPE_HALFPIPE_INVISIBLE_WALL

    /**************
     * BITFIELD 3
     *************/

    SoftWallSuspension = 97, ///< Sets wheel hitbox to the kart's position after touching soft wall
    SoftWallPush = 99,       ///< Gates whether the kart is pushed away from soft walls
    SoftWallUnlockRotation = 100, ///< The kart is in the "Barrel Roll" state
    HWG = 101,                    ///< The "Horizontal Wall Glitch" is active
    AfterCannon = 102,            ///< The kart is being dropped off after a cannon
    ActionMidZipper = 103,        ///< The kart is in an action after being mid-air from a zipper
    ChargeStartBoost = 104,       ///< The kart is charging the start boost during race countdown
    MovingWaterVertical = 105,    ///< KC last turn vertical water
    EndHalfPipe = 107,            ///< An effect trigger ended the half-pipe state this frame

    /**************
     * BITFIELD 4
     *************/

    AutoDrift = 133, ///< Set if using auto transmission, unset if manual transmission

    FlagMax = 160, ///< Internal. Total number of bits in the status.
};

/// @brief Extended bitfield for the kart's status flags
typedef EGG::TBitFlagExt<static_cast<size_t>(eStatus::FlagMax), eStatus> Status;

} // namespace Kinoko::Kart
