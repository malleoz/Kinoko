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

    /// @brief Describes the current drift state of the kart
    enum class DriftState {
        NotDrifting = 0, ///< No active drift
        ChargingMt = 1,  ///< Drifting and charging a mini-turbo
        ChargedMt = 2,   ///< Drifting and a mini-turbo has been charged
        ChargingSmt = 2, ///< Drifting, MT has been charged, and now charging a super MT
        ChargedSmt = 3,  ///< Drifting and a super mini-turbo has been charged
    };

    /// @brief Represents a bit flag for the type of pads the kart is interacting with
    typedef EGG::TBitFlag<u32, ePadType> PadType;

    KartMove();
    virtual ~KartMove();

    /// @addr{0x8057821C}
    /// @brief Constructs the underlying subsystems for the kart
    virtual void createSubsystems(const KartParam::Stats &stats) {
        m_jump = EGG::egg_new<KartJump>(this);
        m_halfPipe = EGG::egg_new<KartHalfPipe>();
        m_kartScale = EGG::egg_new<KartScale>(stats);
    }

    /// @brief Each frame, looks at player input and kart stats. Saves turn-related info.
    virtual void calcTurn();

    /// @brief Handles the wheelie lifecycle and angular velocity from wheeling
    /// @details For karts, this is a no-op since they cannot wheelie
    virtual void calcWheelie() {}

    /// @brief Initializes the system and computes turn-related members
    virtual void setTurnParams();

    virtual void reset(bool preserveScale, bool preserveFloorCount);

    /// @brief Clears some of the kart's movement state when starting an @enum Action
    virtual void clear();

    /// @brief Gets the lean rotation of the vehicle
    /// @addr{0x8058974C}
    /// @details For karts, this is always 0.0f since they cannot lean.
    [[nodiscard]] virtual f32 leanRot() const {
        return 0.0f;
    }

    void setInitialPhysicsValues(const EGG::Vector3f &position, const EGG::Vector3f &angles);

    /// @addr{0x8057EA50}
    /// @brief Clears the kart's drift state
    void resetDriftManual() {
        m_hopStickX = 0;
        m_hopFrame = 0;
        status().resetBit(eStatus::Hop, eStatus::DriftManual);
        m_driftState = DriftState::NotDrifting;
        m_smtCharge = 0;
        m_mtCharge = 0;
    }

    void calc();
    void calcRespawnTrigger();
    void calcInRespawn();
    void calcRespawnBoost();
    void calcTop();
    void calcAirtimeTop();
    void calcSpecialFloor();
    void calcDirs();
    void calcStickyRoad();
    void calcOffroad();
    void calcRisingWater();

    /// @addr{0x80582694}
    /// @brief Computes the current boost state for the kart
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
    /// @brief Computes the current ramp boost state for the kart
    /// @details Clears @enum eStatus::RampBoost when the ramp boost timer ends.
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

    void calcSsmtCharge();
    bool calcPreDrift();
    void calcAutoDrift();
    void calcManualDrift();
    void startManualDrift();
    void clearDrift();

    /// @addr{0x80582DB4}
    /// @brief Clears the jump pad state for the kart
    void clearJumpPad() {
        m_jumpPadMinSpeed = 0.0f;
        status().resetBit(eStatus::JumpPad);
    }

    /// @addr{0x80582DD8}
    /// @brief Clears the ramp boost state for the kart
    void clearRampBoost() {
        m_rampBoostTimer = 0;
        status().resetBit(eStatus::RampBoost);
    }

    /// @addr{0x80582F38}
    /// @brief Clears the zipper boost state for the kart
    void clearZipperBoost() {
        m_zipperBoostTimer = 0;
        status().resetBit(eStatus::ZipperBoost);
    }

    /// @addr{0x80582D94}
    /// @brief Clears the boost state for the kart
    void clearBoost() {
        m_boost.resetActive();
        status().resetBit(eStatus::Boost);
    }

    /// @addr{0x80582F58}
    /// @brief Clears the standstill mini-turbo (SSMT) state for the kart
    void clearSsmt() {
        m_ssmtCharge = 0;
        m_ssmtLeewayTimer = 0;
        m_ssmtDisableAccelTimer = 0;
        m_flags.resetBit(eFlags::SsmtCharged, eFlags::SsmtLeeway);
    }

    /// @addr{0x80582F7C}
    /// @brief Clears the offroad invincibility state for the kart
    void clearOffroadInvincibility() {
        m_offroadInvincibilityTimer = 0;
        status().resetBit(eStatus::BoostOffroadInvincibility);
    }

    /// @brief Clears the reject road state for the kart
    void clearRejectRoad() {
        status().resetBit(eStatus::RejectRoadTrigger, eStatus::NoSparkInvisibleWall);
    }

    void releaseMt();
    void calcDrift();
    void calcRotation();
    void calcVehicleSpeed();

    /// @addr{0x8057B028}
    /// @brief Applies gravity-induced rolling on slopes when the kart is near stationary
    void calcDeceleration() {
        f32 vel = 0.0f;
        f32 initialVel = 1.0f - m_smoothedUp.y;
        if (EGG::Mathf::abs(m_speed) < 30.0f && m_smoothedUp.y > 0.0f && initialVel > 0.0f) {
            initialVel = std::min(initialVel * 2.0f, 2.0f);
            vel += initialVel;
            vel *= std::min(0.5f, std::max(-0.5f, -bodyForward().y));
        }
        m_speed += vel;
    }

    [[nodiscard]] f32 calcVehicleAcceleration() const;
    void calcSpeed();
    [[nodiscard]] f32 calcWallCollisionSpeedFactor(f32 &walColSeverity);
    void calcWallCollisionStart(f32 walColSeverity);
    void calcStandstillBoostRot();
    void calcDive();

    /// @addr{Inlined in 0x805788DC}
    /// @brief Calculates whether we are charging a standstill mini-turbo
    void calcSsmt() {
        auto &status = KartObjectProxy::status();

        if (EGG::Mathf::abs(m_speed) >= 10.0f || status.onBit(eStatus::Boost, eStatus::RampBoost) ||
                status.offAnyBit(eStatus::Accelerate, eStatus::Brake)) {
            status.resetBit(eStatus::ChargingSSMT);
            return;
        }

        status.setBit(eStatus::ChargingSSMT).resetBit(eStatus::HopStart, eStatus::DriftInput);
    }

    /// @addr{0x80579968}
    /// @brief Calculates velocity and position induced from a drift hop
    /// @details Once the kart begins falling, the hop velocity and position in cleared so that
    /// normal gravity can take over.
    void calcHop() {
        m_hopVelY = m_hopVelY * 0.998f + m_hopGravity;
        m_hopPosY += m_hopVelY;

        if (m_hopPosY < 0.0f) {
            m_hopPosY = 0.0f;
            m_hopVelY = 0.0f;
        }
    }

    /// @addr{0x80579960}
    /// @brief Dispatches to @ref KartReject::calc() to handle reject road physics
    void calcRejectRoad() {
        m_reject.calc();
    }

    bool calcCollisions(f32 radius, f32 scale, EGG::Vector3f &pos, EGG::Vector3f &upLocal,
            const EGG::Vector3f &prevPos, Field::CollisionInfo *colInfo,
            Field::KCLTypeMask *maskOut, Field::KCLTypeMask flags) const;

    void applyForce(f32 force, const EGG::Vector3f &hitDir, bool stop);

    /// @brief Every frame, calculates rotation, EV, and angular velocity for the kart
    /// @param turn The amount the vehicle is turning
    virtual void calcVehicleRotation(f32 turn);

    /// @brief Initializes hop information, resets upwards EV and clears upwards force
    virtual void startHop();

    /// @brief Called when the vehicle hops or initiates a slipdrift. It just cancels wheelies.
    /// @details For the base class, this is a no-op since karts cannot wheelie.
    virtual void onPreDrift() {}

    /// @brief Called when you collide with a wall so that wheelies can be cancelled
    /// @addr{0x80570D20}
    /// @details For the base class, this is a no-op since karts cannot wheelie.
    virtual void onWallCollision() {}

    virtual void calcMtCharge();

    /// @brief Clears boost and offroad invinsibility when the kart goes out of bounds
    /// @addr{0x80583658}
    virtual void initOob() {
        clearBoost();
        clearJumpPad();
        clearRampBoost();
        clearZipperBoost();
        clearSsmt();
        clearOffroadInvincibility();
    }

    /// @brief Returns the % speed boost from wheelies.
    /// @addr{0x8057C3C8}
    /// @details For karts, this is always 0.0f since karts cannot wheelie.
    [[nodiscard]] virtual f32 getWheelieSoftSpeedLimitBonus() const {
        return 0.0f;
    }

    /// @brief Returns whether the vehicle can perform a wheelie
    /// @addr{0x8058758C}
    [[nodiscard]] virtual bool canWheelie() const {
        return false;
    }

    /// @addr{0x8057DA18}
    /// @brief Checks whether the vehicle can perform a hop
    /// @details In the base game this is virtual, but it is not overriden in any derived class, so
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
    /// @details The kart can drift if its speed is at 55% or greater than its base speed.
    /// @note This doesn't bytematch the base game. The base game's implementation uses the >
    /// comparison. In practice, > is only ever used once, whereas >= is used in every other
    /// scenario, so we simplify by writing this function for the more common scenario.
    bool canStartDrift() const {
        return m_speed >= MINIMUM_DRIFT_THRESOLD * m_baseSpeed;
    }

    /// @addr{Inlined at 0x80587590}
    /// @brief Activates a boost panel boost if the kart is not in an action or preparing to respawn
    /// @details Also sets 60 frames of offroad invinsibility so the kart does not lose speed.
    void tryStartBoostPanel() {
        constexpr s16 BOOST_PANEL_DURATION = 60;

        if (status().onBit(eStatus::BeforeRespawn, eStatus::InAction)) {
            return;
        }

        activateBoost(KartBoost::Type::MushroomAndBoostPanel, BOOST_PANEL_DURATION);
        setOffroadInvincibility(BOOST_PANEL_DURATION);
    }

    /// @addr{Inlined at 0x80587590}
    /// @brief Activates a boost ramp boost if the kart is not in an action or preparing to respawn
    /// @details Also sets 60 frames of offroad invincibility and sets @enum eStatus::RampBoost.
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

    void tryStartJumpPad();
    void tryEndJumpPad();

    /// @addr{0x80582DB4}
    /// @brief Cancels the jump pad effect on the kart
    void cancelJumpPad() {
        m_jumpPadMinSpeed = 0.0f;
        status().resetBit(eStatus::JumpPad);
    }

    /// @addr{0x8057F090}
    /// @brief Activates a boost of the specified type for a set number of frames
    /// @param type The type of boost to activate
    /// @param frames The number of frames to apply the boost for
    /// @details Also sets the @enum eStatus::Boost flag if the boost is successfully activated.
    void activateBoost(KartBoost::Type type, s16 frames) {
        if (m_boost.activate(type, frames)) {
            status().setBit(eStatus::Boost);
        }
    }

    /// @addr{0x8058212C}
    /// @brief Activates the race start boost for a set number of frames
    /// @param frames The number of frames to apply the start boost for
    void applyStartBoost(s16 frames) {
        activateBoost(KartBoost::Type::MiniTurbo, frames);
    }

    void activateMushroom();
    void activateZipperBoost();

    /// @addr{0x805824C8}
    /// @brief Ignores offroad KCL collision for a set amount of time
    /// @param timer Number of frames to ignore offroad slowdown for
    /// @details Also sets the @enum eStatus::BoostOffroadInvincibility flag.
    void setOffroadInvincibility(s16 timer) {
        if (timer > m_offroadInvincibilityTimer) {
            m_offroadInvincibilityTimer = timer;
        }

        status().setBit(eStatus::BoostOffroadInvincibility);
    }

    /// @addr{0x805824F0}
    /// @brief Checks a timer to see if we are still ignoring offroad slowdown
    /// @details When the timer expires, resets the @enum eStatus::BoostOffroadInvincibility flag.
    void calcOffroadInvincibility() {
        auto &status = KartObjectProxy::status();

        if (status.offBit(eStatus::BoostOffroadInvincibility)) {
            return;
        }

        if (--m_offroadInvincibilityTimer > 0) {
            return;
        }

        status.resetBit(eStatus::BoostOffroadInvincibility);
    }

    /// @brief Checks a timer to see if we are still boosting from a mushroom
    /// @details When the timer expires, resets the @enum eStatus::MushroomBoost flag.
    void calcMushroomBoost() {
        auto &status = KartObjectProxy::status();

        if (status.offBit(eStatus::MushroomBoost)) {
            return;
        }

        if (--m_mushroomBoostTimer > 0) {
            return;
        }

        status.resetBit(eStatus::MushroomBoost);
    }

    void calcZipperBoost();

    /// @addr{0x8057F7A8}
    /// @brief Activates a trick boost whose duration depends on vehicle type and jump pad variant
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
    void activateCrush(u16 timer) {
        status().setBit(eStatus::Crushed);
        m_crushTimer = timer;
        m_kartScale->startCrush();
    }

    /// @addr{0x80580F9C}
    /// @brief Checks if the crush effect is still active and updates the timer
    /// @details When the timer expires, resets the @enum eStatus::Crushed flag and re-scales the
    /// kart.
    void calcCrushed() {
        if (status().offBit(eStatus::Crushed)) {
            return;
        }

        if (--m_crushTimer == 0) {
            status().resetBit(eStatus::Crushed);
            m_kartScale->endCrush();
        }
    }

    void calcScale();

    /// @addr{0x80580768}
    /// @brief Activates the shrink effect for 300 frames
    void activateShrink() {
        constexpr u16 SHRINK_DURATION = 300;

        applyShrink(SHRINK_DURATION);
    }

    void applyShrink(u16 timer);

    /// @addr{0x80580998}
    /// @brief Calculates the shock's speed multiplier and checks if the shock should end
    void calcShock() {
        auto &status = state()->status();

        if (status.onBit(eStatus::Shocked)) {
            if (--m_shockTimer == 0) {
                deactivateShock(false);
            }

            m_shockSpeedMultiplier = std::max(0.7f, m_shockSpeedMultiplier - 0.03f);
        } else {
            m_shockSpeedMultiplier = std::min(1.0f, m_shockSpeedMultiplier + 0.05f);
        }
    }

    /// @addr{0x80580A84}
    /// @brief Deactivates the shock effect and optionally resets the speed multiplier
    /// @param resetSpeed If true, resets the speed multiplier to 1.0f
    void deactivateShock(bool resetSpeed) {
        status().resetBit(eStatus::Shocked);
        m_shockTimer = 0;
        m_kartScale->endShrink(0);

        if (resetSpeed) {
            m_shockSpeedMultiplier = 1.0f;
        }
    }

    void enterCannon();
    void calcCannon();
    void calcRotCannon(const EGG::Vector3f &forward);
    void exitCannon();

    /// @addr{0x805799AC}
    /// @brief Called when the screen wipes to black during a respawn
    /// @details Resets the respawn framecounter and sets the @enum eStatus::TriggerRespawn flag.
    void triggerRespawn() {
        m_timeInRespawn = 0;
        status().setBit(eStatus::TriggerRespawn);
    }

    /// @beginSetters
    void setSpeed(f32 val) {
        m_speed = val;
    }

    void setSmoothedUp(const EGG::Vector3f &v) {
        m_smoothedUp = v;
    }

    void setUp(const EGG::Vector3f &v) {
        m_up = v;
    }

    void setDir(const EGG::Vector3f &v) {
        m_dir = v;
    }

    void setVel1Dir(const EGG::Vector3f &v) {
        m_intVelDir = v;
    }

    void setFloorCollisionCount(u16 count) {
        m_floorCollisionCount = count;
    }

    void setKCLWheelSpeedFactor(f32 val) {
        m_kclWheelSpeedFactor = val;
    }

    void setKCLWheelRotFactor(f32 val) {
        m_kclWheelRotFactor = val;
    }

    /// @addr{0x8057B9AC}
    void setKartSpeedLimit() {
        constexpr f32 LIMIT = 120.0f;
        m_hardSpeedLimit = LIMIT;
    }

    /// @addr{0x80581720}
    void setScale(const EGG::Vector3f &v) {
        m_scale = v;
    }

    void setPadType(PadType type) {
        m_padType = type;
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] DriftState driftState() const {
        return m_driftState;
    }

    [[nodiscard]] u16 mtCharge() const {
        return m_mtCharge;
    }

    [[nodiscard]] f32 kclSpeedFactor() const {
        return m_kclSpeedFactor;
    }

    [[nodiscard]] f32 kclRotFactor() const {
        return m_kclRotFactor;
    }

    /// @addr{0x8057EFF8}
    /// @brief Factors in vehicle speed to retrieve our hop direction and magnitude
    /// @return 0.0f if we are too slow to drift, otherwise the hop direction
    [[nodiscard]] s32 getAppliedHopStickX() const {
        return canStartDrift() ? m_hopStickX : 0;
    }

    [[nodiscard]] f32 softSpeedLimit() const {
        return m_softSpeedLimit;
    }

    [[nodiscard]] f32 speed() const {
        return m_speed;
    }

    [[nodiscard]] f32 acceleration() const {
        return m_acceleration;
    }

    [[nodiscard]] const EGG::Vector3f &scale() const {
        return m_scale;
    }

    [[nodiscard]] f32 hardSpeedLimit() const {
        return m_hardSpeedLimit;
    }

    [[nodiscard]] const EGG::Vector3f &smoothedUp() const {
        return m_smoothedUp;
    }

    [[nodiscard]] const EGG::Vector3f &up() const {
        return m_up;
    }

    [[nodiscard]] f32 totalScale() const {
        return m_totalScale;
    }

    [[nodiscard]] f32 hitboxScale() const {
        return m_hitboxScale;
    }

    [[nodiscard]] const EGG::Vector3f &dir() const {
        return m_dir;
    }

    [[nodiscard]] const EGG::Vector3f &lastDir() const {
        return m_lastDir;
    }

    [[nodiscard]] const EGG::Vector3f &intVelDir() const {
        return m_intVelDir;
    }

    [[nodiscard]] const EGG::Vector3f &smoothedForward() const {
        return m_smoothedForward;
    }

    [[nodiscard]] f32 speedRatioCapped() const {
        return m_speedRatioCapped;
    }

    [[nodiscard]] f32 speedRatio() const {
        return m_speedRatio;
    }

    [[nodiscard]] u16 floorCollisionCount() const {
        return m_floorCollisionCount;
    }

    [[nodiscard]] s32 hopStickX() const {
        return m_hopStickX;
    }

    [[nodiscard]] f32 hopPosY() const {
        return m_hopPosY;
    }

    [[nodiscard]] s16 respawnBoostTimer() const {
        return m_respawnBoostTimer;
    }

    [[nodiscard]] s16 respawnPostLandTimer() const {
        return m_respawnPostLandTimer;
    }

    [[nodiscard]] PadType &padType() {
        return m_padType;
    }

    [[nodiscard]] KartJump *jump() const {
        return m_jump;
    }

    [[nodiscard]] KartHalfPipe *halfPipe() const {
        return m_halfPipe;
    }

    [[nodiscard]] KartScale *kartScale() const {
        return m_kartScale;
    }

    [[nodiscard]] KartBurnout &burnout() {
        return m_burnout;
    }
    /// @endGetters

    /// @addr{0x805879A4}
    /// @brief Calculates the interpolation factor when rotating from one orientation to another
    /// @param scale The scale factor for the rotation
    /// @param from The starting orientation
    /// @param to The target orientation
    /// @return The interpolation factor to use for slerping between the two orientations
    [[nodiscard]] static f32 CalcSlerpRate(f32 scale, const EGG::Quatf &from,
            const EGG::Quatf &to) {
        f32 dotNorm = std::max(-1.0f, std::min(1.0f, from.dot(to)));
        f32 acos = EGG::Mathf::acos(dotNorm);
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

    /// @brief Bitfield representing the various flags defined in @enum eFlags
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

    f32 m_baseSpeed;            ///< The base driving speed for the current character/vehicle stats
    f32 m_softSpeedLimit;       ///< Base speed + boosts + wheelies, clamped to the hard speed limit
    f32 m_speed;                ///< Current speed, restricted to the soft speed limit
    f32 m_lastSpeed;            ///< Last frame's speed, cached to calculate angular velocity
    f32 m_hardSpeedLimit;       ///< Absolute speed cap. It's 120, unless you're in a bullet (140)
    f32 m_acceleration;         ///< Captures the acceleration from player input and boosts
    f32 m_speedDragMultiplier;  ///< After 5 frames of airtime, this causes speed to slowly decay
    EGG::Vector3f m_smoothedUp; ///< A smoothed up vector, mostly used after significant airtime
    EGG::Vector3f m_up;         ///< Vector pointing upwards based on the kart's orientation
    EGG::Vector3f m_landingDir; ///< Facing direction that is snapped on landing
    EGG::Vector3f m_dir;        ///< Current facing direction
    EGG::Vector3f m_lastDir;    ///< Signed magnitude direction from last frame's speed
    EGG::Vector3f m_intVelDir;  ///< Direction used to compute the kart's internal velocity
    EGG::Vector3f m_smoothedForward; ///< Smoothed forward vector
    EGG::Vector3f m_dirDiff;         ///< Accumulated difference used when interpolating @ref m_dir
    bool m_hasLandingDir;            ///< Whether @ref m_landingDir was set this frame
    f32 m_outsideDriftAngle;         ///< The facing angle of an outward-drifting vehicle
    f32 m_landingAngle;              ///< Angle correction that decays after landing
    EGG::Vector3f m_outsideDriftLastDir; ///< The prior frame's outward-drifting facing direction
    f32 m_speedRatioCapped;              ///< @ref m_speedRatio but capped at 1.0f.
    f32 m_speedRatio;                    ///< Ratio between current speed and @ref m_baseSpeed
    f32 m_kclSpeedFactor;                ///< Scales the kart's offroad speed [0.0f, 1.0f]
    f32 m_kclRotFactor;                  ///< Scales the kart's offroad turning radius [0.0f, 1.0f]
    f32 m_kclWheelSpeedFactor; ///< The slowest speed multiplier of each wheel's floor collision
    f32 m_kclWheelRotFactor;   ///< The slowest rotation multiplier of each wheel's floor collision
    u16 m_floorCollisionCount; ///< The number of tires colliding with the floor
    s32 m_hopStickX;           ///< The direction of an active hop (-1, 0, or 1)
    s32 m_hopFrame;            ///< A timer that prevents subsequent hops until reset
    EGG::Vector3f m_hopUp;     ///< The up vector when beginning a drift hop
    EGG::Vector3f m_hopDir;    ///< The direction vector when beginning a drift hop
    f32 m_divingRot;           ///< Pitcha velocity based on up/down stick inputs
    f32 m_standStillBoostRot;  ///< Pitch velocity during stand-still boost charging
    DriftState m_driftState;   ///< The current drift state of the kart
    u16 m_mtCharge;            ///< The current MT charge [0, 270]
    u16 m_smtCharge;           ///< The current SMT charge [0, 300]
    f32 m_outsideDriftBonus;   ///< Added to angular velocity when outside drifting.
    KartBoost m_boost;         ///< State management for boosts
    s16 m_zipperBoostTimer;    ///< Number of frames in a boost from a zipper
    s16 m_zipperBoostDuration; ///< Total duration of the active boost from a zipper
    KartReject m_reject;       ///< State management for rejected road collisions
    s16 m_offroadInvincibilityTimer; ///< How many frames until the player is affected by offroad
    s16 m_ssmtCharge;            ///< Increments every frame up to 75 when charging stand-still MT
    s16 m_ssmtLeewayTimer;       ///< Frames to forgive letting go of A before clearing SSMT charge
    s16 m_ssmtDisableAccelTimer; ///< Counter that tracks delay before starting to reverse
    f32 m_realTurn;        ///< The "true" turn magnitude (@ref m_weightedTurn if not drifting)
    f32 m_weightedTurn;    ///< Magnitude+direction of stick input, factoring in the kart's stats
    EGG::Vector3f m_scale; ///< The kart's per-axis scale (pertains to getting crushed)
    f32 m_totalScale;      ///< The kart's overall scale factor (pertains to shocks)
    f32 m_hitboxScale;     ///< The kart's hitbox scale factor (pertains to shocks)
    f32 m_shockSpeedMultiplier;   ///< Speed modifier when shocked
    u16 m_mushroomBoostTimer;     ///< Number of frames until the mushroom boost runs out
    f32 m_invScale;               ///< Inverse of the Z-component of @ref m_scale
    u16 m_shockTimer;             ///< Frames remaining in the shocked state
    u16 m_crushTimer;             ///< Frames remaining in the crushed state
    u32 m_nonZipperAirtime;       ///< Number of frames of airtime not pertaining to zippers
    f32 m_jumpPadMinSpeed;        ///< The minimum travel speed of the kart for the current jump pad
    f32 m_jumpPadMaxSpeed;        ///< The maximum travel speed of the kart for the current jump pad
    f32 m_jumpPadBoostMultiplier; ///< Speed multiplier applied when activating the current jump pad
    f32 m_jumpPadSoftSpeedLimit;  ///< Based on the active boost's speed limit and KCL speed factor
    const JumpPadProperties *m_jumpPadProperties; ///< Pointer to the active jump pad's properties
    u16 m_rampBoostTimer;                         ///< Frames remaining for the active ramp boost
    f32 m_autoDriftAngle;             ///< Lean angle during automatic transmission drifts
    s16 m_autoDriftStartFrameCounter; ///< Number of frames a drift-worthy stick input has been held
    f32 m_cannonLength;               ///< Length from the kart's cannon entry to the cannon exit
    EGG::Vector3f m_cannonEntryPos;   ///< Position of the kart upon entering a cannon
    EGG::Vector3f m_cannonDir;        ///< The direction of the kart while in the cannon
    EGG::Vector3f m_cannonOrthog;     ///< Vertical unit vector orthogonal to the cannon's direction
    EGG::Vector3f m_cannonProgress;   ///< Distance traveled so far in the cannon
    f32 m_hopVelY;                    ///< Current relative velocity due to a hop
    f32 m_hopPosY;                    ///< Current relative position due to a hop
    f32 m_hopGravity;                 ///< Always main gravity (-1.3f)
    s16 m_timeInRespawn;              ///< Number of frames elapsed after position snap from respawn
    s16 m_respawnPreLandTimer;        ///< 4 frame respawn boost leniency timer before landing
    s16 m_respawnPostLandTimer;       ///< 4 frame respawn boost leniency timer after landing
    s16 m_respawnBoostTimer;          ///< Number of frames until the respawn boost runs out
    s16 m_bumpTimer;                  ///< Cooldown after a @enum Reaction::SmallBump collision
    DrivingDirection m_drivingDirection; ///< Current state of the kart's driving direction
    s16 m_backwardsAllowCounter;         ///< Tracks the 15f delay before reversing
    PadType m_padType;                   ///< Bitfield of the pad types currently active
    Flags m_flags;                       ///< Bitfield of various movement-related kart flags
    KartJump *m_jump;                    ///< Pointer to the underlying @ref KartJump subsystem
    KartHalfPipe *m_halfPipe;            ///< Pointer to the underlying @ref KartHalfPipe subsystem
    KartScale *m_kartScale;              ///< Pointer to the underlying @ref KartScale subsystem
    KartBurnout m_burnout;               ///< Manages the state of start boost burnout
    const DriftingParameters *m_driftingParams; ///< Pointer to the kart's drift parameters
    f32 m_rawTurn;                              ///< Raw stick magnitude and direction [-1.0f, 1.0f]

    /// @brief Minimum base speed percentage required to initiate a drift
    static constexpr f32 MINIMUM_DRIFT_THRESOLD = 0.55f;

    /// @brief The cannon parameters, indexed by @ref System::MapdataCannonPoint::m_parameterIdx
    static constexpr std::array<CannonParameter, 3> CANNON_PARAMETERS = {{
            {500.0f, 0.0f, 6000.0f, -1.0f},
            {500.0f, 5000.0f, 6000.0f, -1.0f},
            {120.0f, 2000.0f, 1000.0f, 45.0f},
    }};
};

/// @copybrief KartMove
/// @details This derived class has specialized behavior for bikes, such as wheelies and leaning.
/// There are also additional member variables to track the bike's unique state.
class KartMoveBike : public KartMove {
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

    void startWheelie();

    /// @addr{0x805883C4}
    /// @brief Clears the wheelie bit flag and resets the rotation decrement
    /// @details In the base game this is a virtual function, but since no derived class overrides
    /// it here, we can divirtualize for Kinoko.
    void cancelWheelie() {
        status().resetBit(eStatus::Wheelie);
        m_wheelieRotDec = 0.0f;
        m_autoHardStickXFrames = 0;
    }

    /// @copybrief KartMove::createSubsystems()
    /// @addr{0x80587BB8}
    /// @details This override creates the bike-specific @ref KartJumpBike subsystem.
    void createSubsystems(const KartParam::Stats &stats) override {
        m_jump = EGG::egg_new<KartJumpBike>(this);
        m_halfPipe = EGG::egg_new<KartHalfPipe>();
        m_kartScale = EGG::egg_new<KartScale>(stats);
    }

    void calcVehicleRotation(f32 /*turn*/) override;
    void calcWheelie() override;

    /// @copybrief KartMove::onPreDrift()
    /// @addr{0x80588B30}
    void onPreDrift() override {
        if (status().onBit(eStatus::AutoDrift)) {
            return;
        }

        cancelWheelie();
    }

    /// @copybrief KartMove::onWallCollision()
    /// @addr{0x80588FC0}
    /// @details Simply cancels wheelies on wall collision.
    void onWallCollision() override {
        cancelWheelie();
    }

    void calcMtCharge() override;

    /// @copybrief KartMove::initOob()
    /// @addr{0x80588B58}
    /// @details For bikes, this also cancels wheelies.
    void initOob() override {
        KartMove::initOob();
        cancelWheelie();
    }

    void setTurnParams() override;
    void reset(bool preserveScale, bool preserveFloorCount) override;

    /// @addr{0x80588950}
    /// @copybrief KartMove::clear()
    /// @details Also cancels wheelies.
    void clear() override {
        KartMove::clear();
        cancelWheelie();
    }

    /// @addr{0x80588324}
    /// @copybrief KartMove::getWheelieSoftSpeedLimitBonus()
    /// @details Applies a 15% speed increase when the bike is performing a wheelie.
    [[nodiscard]] f32 getWheelieSoftSpeedLimitBonus() const override {
        constexpr f32 WHEELIE_SPEED_BONUS = 0.15f;
        return status().onBit(eStatus::Wheelie) ? WHEELIE_SPEED_BONUS : 0.0f;
    }

    /// @addr{0x80588860}
    /// @brief Limits the kart's yaw rotation while performing a wheelie
    [[nodiscard]] f32 wheelieRotFactor() const {
        constexpr f32 WHEELIE_ROTATION_FACTOR = 0.2f;

        return status().onBit(eStatus::Wheelie) ? WHEELIE_ROTATION_FACTOR : 1.0f;
    }

    void calcWheelieInput();

    /// @beginGetters
    /// @addr{0x805896BC}
    /// @copybrief KartMove::leanRot()
    [[nodiscard]] f32 leanRot() const override {
        return m_leanRot;
    }

    /// @addr{0x80588FE0}
    /// @copybrief KartMove::canWheelie()
    /// @details The kart can wheelie if it is traveling over 30% of its base speed stat
    [[nodiscard]] bool canWheelie() const override {
        constexpr f32 WHEELIE_THRESHOLD = 0.3f;

        return m_speedRatioCapped >= WHEELIE_THRESHOLD && m_speed >= 0.0f;
    }
    /// @endGetters

private:
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
