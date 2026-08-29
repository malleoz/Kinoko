#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Kart {

/// @brief Represents an action applied to the kart due to a collision with an object
enum class Action {
    None = -1,         ///< No action is being applied to the kart
    SpinOnce = 0,      ///< The kart spins out, rotating once
    SpinTwice = 1,     ///< The kart spins out, rotating twice
    ForwardLaunch = 2, ///< The kart launches forward into the air and flips once
    AwayFlipOnce = 3,  ///< The kart launches into the air away from the collision and flips once
    AwayFlipTwice = 4, ///< The kart launches into the air away from the collision and flips twice
    SidewaysFlipTwice = 5,   ///< The kart launches sideways into the air and flips twice
    LaunchSpinLoseItem = 6,  ///< The kart launches into the air, flips once, and loses its items
    ExplosionLoseItem = 7,   ///< The kart launches high into the air, and loses its items
    HighLaunchLoseItem = 8,  ///< The kart launches high into the air, and loses its items
    FireSpin = 9,            ///< The kart spins out due to a fireball collision
    LongCrushLoseItem = 12,  ///< The kart is crushed for 8 seconds and loses its items
    ShortCrushLoseItem = 14, ///< The kart is crushed for 4 seconds and loses its items
    SpinShrink = 15,         ///< The kart spins out and shrinks in size temporarily
    CrushRespawn = 16,       ///< The kart is crushed and respawns
    Max = 18,                ///< The maximum number of actions, used for array sizing
};

/// @brief Manages the state of a kart when affected by an action induced by an object collision
/// @details An action begins when another @ref Kart class calls @ref start(). Every frame an action
/// is active, @ref calc() will update the kart's position, rotation, etc. based on action-specific
/// logic. Once the action is complete, @ref calc() will call the associated end function and clear
/// the action.
class KartAction : private KartObjectProxy {
public:
    /// @brief Flags that represent the current state of the active action
    enum class eFlags {
        Landing = 0,    ///< The kart has reached the target rotation and is touching the ground
        FlipBounce = 2, ///< Set once the kart bounces off the floor in between flips
        Rotating = 3,   ///< Set while the kart is rotating due to the current action
        LandingFromFlip = 5, ///< Touching the ground after a large flip action
    };

    /// @brief A bitfield of @ref eFlags that represents the state of the current action
    typedef EGG::TBitFlag<u32, eFlags> Flags;

    KartAction();
    ~KartAction();

    /// @addr{0x8056739C}
    /// @brief Resets the current action and clears all flags
    void init() {
        m_currentAction = Action::None;
        m_flags.makeAllZero();
    }

    void calc();
    void calcVehicleSpeed();
    bool start(Action action);
    void startRotation(size_t idx);

    /// @beginSetters
    void setHitDepth(const EGG::Vector3f &hitDepth) {
        m_hitDepth = hitDepth;
    }

    void setVelocity(const EGG::Vector3f &v) {
        m_velocity = v;
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] const Flags &flags() const {
        return m_flags;
    }
    /// @endGetters

private:
    /// @brief Parameters specific to an action ID
    struct ActionParams {
        f32 startSpeedMult; ///< Multiplier applied to the kart's speed at the start of the action
        f32 calcSpeedMult;  ///< Decays the kart's speed every frame while the action is active
        s16 priority;       ///< Determines if other actions can start while this action is active
    };

    /// @brief Parameters that control the rotation of the kart during certain actions
    struct RotationParams {
        f32 initRotSpeed;  ///< Initial rotation speed when starting the action
        f32 minRotSpeed;   ///< Minimum speed to clamp rotation to once it decays near end of action
        f32 minDecayRate;  ///< Floor for the rotation decay rate
        f32 initDecayRate; ///< Initial decay rate for the rotation speed when starting the action
        f32 slowdownThreshold; ///< The % of total rotation at which to begin slowing the rotation
        f32 finalAngle;        ///< The total rotation angle to reach before the action ends
    };

    /// @brief Function pointer type for a function that should run when entering an action
    /// @note The player index sent into StartActionFunc is assumed to be cosmetic, so we do not
    /// implement it here.
    typedef void (KartAction::*StartActionFunc)();

    /// @brief Function pointer type for a function that should run every frame while in an action
    typedef bool (KartAction::*CalcActionFunc)();

    /// @brief Function pointer type for a function that should run once an action has ended
    /// @param newActionStarting Whether or not this action ended because a higher priority action
    /// is now starting
    typedef void (KartAction::*EndActionFunc)(bool newActionStarting);

    void calcSideFromHitDepth();
    void calcSideFromHitDepthAndTranslation();

    void end();

    /// @addr{0x80567A54}
    /// @brief Executes a frame of the current action.
    /// @return Whether or not the action should end.
    bool calcCurrentAction() {
        ++m_frame;
        return (this->*m_onCalc)();
    }

    void calcEndAction(bool newActionStarting);
    bool calcRotation();
    void calcUp();
    void calcLanding();
    void startLaunch(f32 extVelScalar, f32 extVelKart, f32 extVelBike, f32 numRotations,
            u32 param6);
    void activateCrush(u16 timer);

    void applyStartSpeed();
    void setRotation(size_t idx);

    /* ================================ *
     *     START FUNCTIONS
     * ================================ */

    void startStub() {}

    /// @addr{0x80567FB4}
    /// @brief Called when the kart begins to spin out and rotates 720 degrees
    void startSpinTwice() {
        startRotation(2);
    }

    void startSmallLaunch();
    void startActionAwayFlipOnce();
    void startActionAwayFlipTwice();
    void startActionSidewaysFlipTwice();
    void startLargeFlipAction();

    /// @addr{0x80568000}
    /// @brief Called when the kart begins to spin out from a fireball and rotates 720 degrees
    void startFireSpin() {
        startRotation(2);
    }

    void startLongPressAction();
    void startShortPressAction();

    /// @addr{0x80568058}
    /// @brief Called when the kart begins to spin out and shrink in size temporarily
    void startSpinShrinkAction() {
        startRotation(1);
    }

    /* ================================ *
     *     CALC FUNCTIONS
     * ================================ */

    bool calcStub() {
        return false;
    }

    bool calcSpin();
    bool calcLaunchAction();
    bool calcActionAwayFlipTwice();
    bool calcLargeFlipAction();
    bool calcPressAction();

    /* ================================ *
     *     END FUNCTIONS
     * ================================ */

    void endStub(bool /*newActionStarting*/) {}

    void endSpin(bool newActionStarting);
    void endLaunchAction(bool newActionStarting);

    EGG::Vector3f m_launchDir; ///< Direction that the kart is launched towards
    Action m_currentAction;    ///< The current action being applied to the kart
    f32 m_rotationSide;        ///< 1.0f is rotating counter-clockwise, -1.0f if rotating clockwise
    f32 m_targetRot; ///< Total rotation that @ref calcLanding() waits for before allowing landing
    EGG::Vector3f m_hitDepth; ///< The depth of the collision that caused a launch action to start
    EGG::Vector3f m_rotAxis;  ///< The axis that the kart rotates around during launch actions
    EGG::Vector3f m_velocity; ///< Velocity of the colliding object
    f32 m_pitchRotVel;        ///< The current per-frame rate at which the kart's pitch changes
    f32 m_pitch;              ///< The current pitch of the kart during a large flip action
    f32 m_pitchTraveled;      ///< Cumulative pitch traveled so far
    f32 m_wobblePhase;        ///< Wobble cycle phase that affects side-to-side rotation mid-flip
    s32 m_groundStartLaunchTimer; ///< Frames elapsed since the start of a ground-start double flip
    StartActionFunc m_onStart;    ///< Function that runs when the current action starts
    CalcActionFunc m_onCalc;      ///< Function that runs every frame for the current action
    EndActionFunc m_onEnd;        ///< Function that runs when the current action ends
    EGG::Quatf m_rotation;        ///< Incremental rotation quaternion applies to the kart
    const ActionParams *m_actionParams; ///< Pointer to the parameters for the current action
    u32 m_frame;               ///< Number of frames elapsed since the current action started
    u32 m_crushActionDuration; ///< Total frame duration of the current crush action
    Flags m_flags;             ///< Bit flags that represent the current state of the active action
    f32 m_yawAngle;            ///< Accumulated rotation angle of the kart when spinning out
    f32 m_yawRotVel;           ///< The current per-frame rate at which the kart's yaw changes
    f32 m_decayRate;      ///< Decay factor for the kart's yaw rotation velocity while spinning out
    f32 m_decayRateDelta; ///< Subtracted from @ref m_decayRate every frame
    f32 m_targetYaw; ///< Target total rotation when the kart is spinning out for the action to end
    const RotationParams *m_rotationParams; ///< Pointer to params for the current spinout action
    EGG::Vector3f m_up;                     ///< Smoothed up vector of the kart
    u16 m_flipBounceFrames; ///< Frames elapsed in the post-bounce phase of a large flip action
    s16 m_priority; ///< Current action's priority. Only higher priority actions can interrupt it.

    /* ================================ *
     *     ACTION TABLES
     * ================================ */

    // @brief The maximum number of actions, used for array sizing
    static constexpr size_t MAX_ACTION = static_cast<size_t>(Action::Max);

    /// @addr{0x808B4C58}
    /// @brief Parameters for each action, indexed by the Action enum
    static constexpr std::array<ActionParams, MAX_ACTION> ACTION_PARAMS = {{
            {0.98f, 0.98f, 1}, ///< Action::SpinOnce
            {0.98f, 0.98f, 2}, ///< Action::SpinTwice
            {0.96f, 0.96f, 4}, ///< Action::ForwardLaunch
            {0.0f, 0.96f, 4},  ///< Action::AwayFlipOnce
            {0.0f, 0.98f, 4},  ///< Action::AwayFlipTwice
            {0.0f, 0.96f, 4},  ///< Action::SidewaysFlipTwice
            {0.0f, 0.96f, 4},  ///< Action::LaunchSpinLoseItem
            {0.0f, 0.0f, 6},   ///< Action::ExplosionLoseItem
            {0.0f, 0.99f, 6},  ///< Action::HighLaunchLoseItem
            {0.98f, 0.98f, 3}, ///< Action::FireSpin
            {0.98f, 0.98f, 3}, ///< Unused in Kinoko
            {1.0f, 1.0f, 5},   ///< Unused in Kinoko
            {0.0f, 0.0f, 3},   ///< Action::LongCrushLoseItem
            {0.0f, 0.0f, 3},   ///< Unused in Kinoko
            {0.0f, 0.0f, 3},   ///< Action::ShortCrushLoseItem
            {0.98f, 0.98f, 3}, ///< Action::SpinShrink
            {0.0f, 0.0f, 3},   ///< Action::CrushRespawn
            {0.98f, 0.98f, 3}, ///< Unused in Kinoko
    }};

    /// @addr{0x80891550}
    /// @brief Rotation parameters for spinout actions
    /// @details The first three entries are for spinout actions, and the last two are for large
    /// flip actions.
    static constexpr std::array<RotationParams, 5> ROTATION_PARAMS = {{
            {10.0f, 1.5f, 0.9f, 0.005f, 0.6f, 360.0f},
            {11.0f, 1.5f, 0.9f, 0.0028f, 0.7f, 720.0f},
            {11.0f, 1.5f, 0.9f, 0.0028f, 0.8f, 1080.0f},
            {7.0f, 1.5f, 0.9f, 0.005f, 0.6f, 450.0f},
            {9.0f, 1.5f, 0.9f, 0.0028f, 0.7f, 810.0f},
    }};

    /// @addr{0x808B4D40}
    /// @brief Function pointers for each action's start function, indexed by the Action enum
    static constexpr std::array<StartActionFunc, MAX_ACTION> ON_START = {{
            &KartAction::startStub,                    ///< @enum Action::SpinOnce
            &KartAction::startSpinTwice,               ///< @enum Action::SpinTwice
            &KartAction::startSmallLaunch,             ///< @enum Action::ForwardLaunch
            &KartAction::startActionAwayFlipOnce,      ///< @enum Action::AwayFlipOnce
            &KartAction::startActionAwayFlipTwice,     ///< @enum Action::AwayFlipTwice
            &KartAction::startActionSidewaysFlipTwice, ///< @enum Action::SidewaysFlipTwice
            &KartAction::startStub,                    ///< @enum Action::LaunchSpinLoseItem
            &KartAction::startLargeFlipAction,         ///< @enum Action::ExplosionLoseItem
            &KartAction::startLargeFlipAction,         ///< @enum Action::HighLaunchLoseItem
            &KartAction::startFireSpin,                ///< @enum Action::FireSpin
            &KartAction::startStub,                    ///< Unused in Kinoko
            &KartAction::startStub,                    ///< Unused in Kinoko
            &KartAction::startLongPressAction,         ///< @enum Action::LongCrushLoseItem
            &KartAction::startStub,                    ///< Unused in Kinoko
            &KartAction::startShortPressAction,        ///< @enum Action::ShortCrushLoseItem
            &KartAction::startSpinShrinkAction,        ///< @enum Action::SpinShrink
            &KartAction::startStub,                    ///< @enum Action::CrushRespawn
            &KartAction::startStub,                    ///< Unused in Kinoko
    }};

    /// @addr{0x808B4E18}
    /// @brief Pointers for each action's per-frame calc function, indexed by the Action enum
    static constexpr std::array<CalcActionFunc, MAX_ACTION> ON_CALC = {{
            &KartAction::calcStub,                ///< @enum Action::SpinOnce
            &KartAction::calcSpin,                ///< @enum Action::SpinTwice
            &KartAction::calcLaunchAction,        ///< @enum Action::ForwardLaunch
            &KartAction::calcLaunchAction,        ///< @enum Action::AwayFlipOnce
            &KartAction::calcActionAwayFlipTwice, ///< @enum Action::AwayFlipTwice
            &KartAction::calcLaunchAction,        ///< @enum Action::SidewaysFlipTwice
            &KartAction::calcStub,                ///< @enum Action::LaunchSpinLoseItem
            &KartAction::calcLargeFlipAction,     ///< @enum Action::ExplosionLoseItem
            &KartAction::calcLargeFlipAction,     ///< @enum Action::HighLaunchLoseItem
            &KartAction::calcSpin,                ///< @enum Action::FireSpin
            &KartAction::calcStub,                ///< Unused in Kinoko
            &KartAction::calcStub,                ///< Unused in Kinoko
            &KartAction::calcPressAction,         ///< @enum Action::LongCrushLoseItem
            &KartAction::calcStub,                ///< Unused in Kinoko
            &KartAction::calcPressAction,         ///< @enum Action::ShortCrushLoseItem
            &KartAction::calcSpin,                ///< @enum Action::SpinShrink
            &KartAction::calcStub,                ///< @enum Action::CrushRespawn
            &KartAction::calcStub,                ///< Unused in Kinoko
    }};

    /// @addr{0x808B4EF0}
    /// @brief Function pointers for each action's end function, indexed by the Action enum
    static constexpr std::array<EndActionFunc, MAX_ACTION> ON_END = {{
            &KartAction::endStub,         ///< @enum Action::SpinOnce
            &KartAction::endSpin,         ///< @enum Action::SpinTwice
            &KartAction::endLaunchAction, ///< @enum Action::ForwardLaunch
            &KartAction::endLaunchAction, ///< @enum Action::AwayFlipOnce
            &KartAction::endLaunchAction, ///< @enum Action::AwayFlipTwice
            &KartAction::endLaunchAction, ///< @enum Action::SidewaysFlipTwice
            &KartAction::endStub,         ///< @enum Action::LaunchSpinLoseItem
            &KartAction::endStub,         ///< @enum Action::ExplosionLoseItem
            &KartAction::endStub,         ///< @enum Action::HighLaunchLoseItem
            &KartAction::endSpin,         ///< @enum Action::FireSpin
            &KartAction::endStub,         ///< Unused in Kinoko
            &KartAction::endStub,         ///< Unused in Kinoko
            &KartAction::endStub,         ///< @enum Action::LongCrushLoseItem
            &KartAction::endStub,         ///< Unused in Kinoko
            &KartAction::endStub,         ///< @enum Action::ShortCrushLoseItem
            &KartAction::endSpin,         ///< @enum Action::SpinShrink
            &KartAction::endStub,         ///< @enum Action::CrushRespawn
            &KartAction::endStub,         ///< Unused in Kinoko
    }};
};

} // namespace Kinoko::Kart
