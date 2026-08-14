#include "KartAction.hh"

#include "game/kart/KartMove.hh"
#include "game/kart/KartPhysics.hh"
#include "game/kart/KartState.hh"

#include "game/item/ItemDirector.hh"
#include "game/item/KartItem.hh"

#include <egg/math/Math.hh>

namespace Kinoko::Kart {

/// @addr{0x805672CC}
KartAction::KartAction()
    : m_currentAction(Action::None), m_hitDepth(EGG::Vector3f::zero), m_velocity(EGG::Vector3f::ez),
      m_onStart(nullptr), m_onCalc(nullptr), m_onEnd(nullptr), m_actionParams(nullptr),
      m_rotationParams(nullptr), m_priority(0) {}

/// @addr{0x8056A1A8}
KartAction::~KartAction() = default;

/// @addr{0x805673B0}
/// @brief Every frame, evaluates the current action (if any) and checks if it has ended
void KartAction::calc() {
    if (m_currentAction == Action::None || !m_onCalc) {
        return;
    }

    if (calcCurrentAction()) {
        calcEndAction(false);
    }
}

/// @addr{0x80567CE4}
/// @brief Decays the kart's speed based off of the current action's speed multiplier
void KartAction::calcVehicleSpeed() {
    move()->setSpeed(m_actionParams->calcSpeedMult * move()->speed());
}

/// @addr{0x805675DC}
/// @brief Starts an action
/// @details Skips all actions if the player is in a cannon or is respawning. Modifies the action if
/// the player is on a zipper. Checks the priority of the current action and the new action to
/// determine if the new action can be started. If so, ends the current action. Then, sets the
/// current action's params and function pointers, and calls the start function for the new action.
/// @param action The action to start
/// @return Whether or not the action was started
bool KartAction::start(Action action) {
    ASSERT(action != Action::None);

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::InRespawn, eStatus::AfterRespawn, eStatus::BeforeRespawn,
                eStatus::InCannon)) {
        return false;
    }

    if (status.onBit(eStatus::ZipperStick)) {
        switch (action) {
        case Action::ForwardLaunch:
        case Action::AwayFlipOnce:
        case Action::AwayFlipTwice:
        case Action::SidewaysFlipTwice:
        case Action::LaunchSpinLoseItem:
            action = Action::SpinTwice;
            break;
        default:
            break;
        }
    }

    size_t actionIdx = static_cast<size_t>(action);

    if (m_currentAction != Action::None && ACTION_PARAMS[actionIdx].priority <= m_priority) {
        return false;
    }

    calcEndAction(true);
    m_currentAction = action;
    m_actionParams = &ACTION_PARAMS[actionIdx];
    m_priority = m_actionParams->priority;
    m_onStart = ON_START[actionIdx];
    m_onCalc = ON_CALC[actionIdx];
    m_onEnd = ON_END[actionIdx];
    status.setBit(eStatus::InAction);
    m_frame = 0;
    m_flags.makeAllZero();
    m_up = move()->up();
    move()->clear();

    applyStartSpeed();
    (this->*m_onStart)();
    return true;
}

/// @addr{0x80567D3C}
/// @brief Initializes rotation parameters when a spinout action begins
/// @warning The parameter is supposed to be an enum, but we discard it.
/// This results in the arguments being off-by-one. Beware of zero!
/// @param idx The index for the rotation parameters.
void KartAction::startRotation(size_t idx) {
    m_rotation = EGG::Quatf::ident;

    EGG::Vector3f dir = move()->dir();
    if (speed() < 0.0f) {
        dir = -dir;
    }

    m_rotationSide = dir.cross(bodyFront()).dot(bodyUp()) > 0.0f ? 1.0f : -1.0f;
    setRotation(idx);
    m_flags.setBit(eFlags::Rotating);
}

/// @addr{0x80569AE8}
/// @brief Computes @ref m_launchDir from @ref m_hitDepth, projected onto the plane perpendicular
/// to the kart's smoothed up vector
void KartAction::calcSideFromHitDepth() {
    m_hitDepth.normalise();
    m_launchDir = m_hitDepth.perpInPlane(move()->smoothedUp(), true);

    if (m_launchDir.squaredLength() <= std::numeric_limits<f32>::epsilon()) {
        m_launchDir = EGG::Vector3f::ey;
    }
}

/// @addr{0x80569B94}
/// @brief Computes @ref m_launchDir like @ref calcSideFromHitDepth, but biased by the colliding
/// object's @ref m_velocity to pick which side of the kart it hit
void KartAction::calcSideFromHitDepthAndTranslation() {
    calcSideFromHitDepth();

    EGG::Vector3f cross = m_velocity.cross(m_launchDir);
    f32 sign = (cross.y > 0.0f) ? 1.0f : -1.0f;

    EGG::Vector3f worldSide = EGG::Vector3f::ey.cross(m_velocity);
    worldSide.normalise();

    m_launchDir = worldSide.perpInPlane(move()->smoothedUp(), true);

    if (m_launchDir.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_launchDir *= sign;
    } else {
        m_launchDir = EGG::Vector3f::ey;
    }
}

/// @addr{0x80567B98}
/// @brief Runs when an action ends, resetting the current action and clearing all flags
void KartAction::end() {
    status().resetBit(eStatus::InAction, eStatus::LargeFlipHit);
    dynamics()->setForceUpright(true);

    m_currentAction = Action::None;
    m_priority = 0;
    m_flags.makeAllZero();
}

/// @addr{0x80567A88}
/// @brief Runs the end function for the current action, if any, and resets the current action
void KartAction::calcEndAction(bool endArg) {
    if (m_currentAction == Action::None) {
        return;
    }

    if (m_onEnd) {
        (this->*m_onEnd)(endArg);
        end();
    }
}

/// @addr{0x80569DFC}
/// @brief Runs every frame while a spinout action is active
/// @details Updates the kart's current yaw to reflect the current yaw rotational velocity from the
/// spinout. Decays rotation speed after reaching the action's slowdown threshold.
bool KartAction::calcRotation() {
    if (!m_rotationParams) {
        return false;
    }

    // Slow the rotation down as we approach the end of the spinout
    if (m_yawAngle > m_targetYaw * m_rotationParams->slowdownThreshold) {
        m_yawRotVel *= m_decayRate;
        if (m_rotationParams->minRotSpeed > m_yawRotVel) {
            m_yawRotVel = m_rotationParams->minRotSpeed;
        }

        m_decayRate -= m_decayRateDelta;
        if (m_rotationParams->minDecayRate > m_decayRate) {
            m_decayRate = m_rotationParams->minDecayRate;
        }
    }

    m_yawAngle += m_yawRotVel;
    if (m_targetYaw < m_yawAngle) {
        m_yawAngle = m_targetYaw;
        return true;
    }

    return false;
}

/// @addr{0x80569E9C}
/// @brief Interpolates the smoothed up vector based off the kart's current up vector
void KartAction::calcUp() {
    constexpr f32 UP_INTERP_RATE = 0.3f;

    m_up += (move()->up() - m_up) * UP_INTERP_RATE;
    m_up.normalise();
}

/// @brief Checks if the kart is landing from a launch and decays the kart's rotation if so
void KartAction::calcLanding() {
    if (m_yawAngle < m_targetRot || status().offBit(eStatus::TouchingGround)) {
        return;
    }

    m_flags.setBit(eFlags::Landing);

    if (!isBike()) {
        dynamics()->setForceUpright(false);
    }

    physics()->composeDecayingExtraRot(m_rotation);
}

/// @addr{0x80568794}
void KartAction::startLaunch(f32 extVelScalar, f32 extVelKart, f32 extVelBike, f32 numRotations,
        u32 param6) {
    m_targetRot = 360.0f * numRotations;

    EGG::Vector3f extVel = EGG::Vector3f::zero;
    extVel.y = isBike() ? extVelBike : extVelKart;

    if (param6 == 0) {
        m_hitDepth = move()->dir();
        calcSideFromHitDepth();
    } else if (param6 == 1) {
        calcSideFromHitDepth();
        extVel += extVelScalar * m_launchDir;
    } else if (param6 == 2) {
        calcSideFromHitDepthAndTranslation();
        extVel += extVelScalar * m_launchDir;
    }

    setRotation(static_cast<size_t>(numRotations + 3.0f));
    m_groundStartLaunchTimer = 0;
    m_rotAxis = move()->smoothedUp().cross(m_launchDir);

    dynamics()->setExtVel(dynamics()->extVel() + extVel);
}

/// @addr{0x805696CC}
void KartAction::activateCrush(u16 timer) {
    move()->activateCrush(m_crushActionDuration + timer);
    Item::ItemDirector::Instance()->kartItem(0).clear();
}

/// @addr{0x80567C68}
void KartAction::applyStartSpeed() {
    move()->setSpeed(m_actionParams->startSpeedMult * move()->speed());
    if (m_actionParams->startSpeedMult == 0.0f) {
        move()->clearDrift();
    }
}

/// @addr{0x80569DB4}
void KartAction::setRotation(size_t idx) {
    ASSERT(idx - 1 < ROTATION_PARAMS.size());
    m_rotationParams = &ROTATION_PARAMS[--idx];

    m_targetYaw = m_rotationParams->finalAngle;
    m_yawRotVel = m_rotationParams->initRotSpeed;
    m_decayRateDelta = m_rotationParams->initDecayRate;
    m_yawAngle = 0.0f;
    m_decayRate = 1.0f;
}

/* ================================ *
 *     START FUNCTIONS
 * ================================ */

/// @addr{0x8056865C}
void KartAction::startSmallLaunch() {
    constexpr f32 EXT_VEL_SCALAR = 0.0f;
    constexpr f32 EXT_VEL_KART = 30.0f;
    constexpr f32 EXT_VEL_BIKE = 30.0f;
    constexpr f32 NUM_ROTATIONS = 1.0f;

    startLaunch(EXT_VEL_SCALAR, EXT_VEL_KART, EXT_VEL_BIKE, NUM_ROTATIONS, 0);
}

/// @addr{0x80568718}
void KartAction::startActionAwayFlipOnce() {
    constexpr f32 EXT_VEL_SCALAR = 25.0f;
    constexpr f32 EXT_VEL_KART = 30.0f;
    constexpr f32 EXT_VEL_BIKE = 30.0f;
    constexpr f32 NUM_ROTATIONS = 1.0f;

    startLaunch(EXT_VEL_SCALAR, EXT_VEL_KART, EXT_VEL_BIKE, NUM_ROTATIONS, 1);
    Item::ItemDirector::Instance()->kartItem(0).clear();
}

/// @addr{0x80568CB8}
void KartAction::startActionAwayFlipTwice() {
    constexpr f32 EXT_VEL_SCALAR = 25.0f;
    constexpr f32 EXT_VEL_KART = 30.0f;
    constexpr f32 EXT_VEL_BIKE = 30.0f;
    constexpr f32 NUM_ROTATIONS = 2.0f;

    startLaunch(EXT_VEL_SCALAR, EXT_VEL_KART, EXT_VEL_BIKE, NUM_ROTATIONS, 1);
    Item::ItemDirector::Instance()->kartItem(0).clear();
}

/// @addr{0x80568FA4}
void KartAction::startActionSidewaysFlipTwice() {
    constexpr f32 EXT_VEL_SCALAR = 13.0f;
    constexpr f32 EXT_VEL_KART = 40.0f;
    constexpr f32 EXT_VEL_BIKE = 45.0f;
    constexpr f32 NUM_ROTATIONS = 1.0f;

    startLaunch(EXT_VEL_SCALAR, EXT_VEL_KART, EXT_VEL_BIKE, NUM_ROTATIONS, 2);
}

/// @addr{0x805690A0}
void KartAction::startLargeFlipAction() {
    constexpr EGG::Vector3f INIT_VEL = EGG::Vector3f(0.0f, 60.0f, 0.0f);
    constexpr f32 INIT_PITCH_VEL = 22.0f;

    dynamics()->setExtVel(INIT_VEL);
    dynamics()->setAngVel0(EGG::Vector3f::zero);

    if (m_currentAction == Action::HighLaunchLoseItem) {
        calcSideFromHitDepth();
        dynamics()->setExtVel(dynamics()->extVel() + m_launchDir * -20.0f);
    }

    Item::ItemDirector::Instance()->kartItem(0).clear();

    m_pitchTraveled = 0.0f;
    m_pitch = 0.0f;
    m_pitchRotVel = INIT_PITCH_VEL;
    m_wobblePhase = 0.0f;
    m_flipBounceFrames = 0;

    status().setBit(eStatus::LargeFlipHit);
}

/// @addr{0x80569774}
void KartAction::startLongPressAction() {
    constexpr u32 ACTION_DURATION = 90;
    constexpr u16 CRUSH_DURATION = 480;

    m_crushActionDuration = ACTION_DURATION;
    activateCrush(CRUSH_DURATION);
}

/// @addr{0x80569978}
void KartAction::startShortPressAction() {
    constexpr u32 ACTION_DURATION = 30;
    constexpr u16 CRUSH_DURATION = 240;

    m_crushActionDuration = ACTION_DURATION;
    activateCrush(CRUSH_DURATION);
}

/* ================================ *
 *     CALC FUNCTIONS
 * ================================ */

/// @addr{0x80568204}
bool KartAction::calcSpin() {
    calcUp();
    bool finished = calcRotation();

    m_rotation.setAxisRotation(DEG2RAD * (m_yawAngle * m_rotationSide), m_up);
    physics()->composeExtraRot(m_rotation);
    return finished;
}

/// @addr{0x80568AA8}
bool KartAction::calcLaunchAction() {
    constexpr u32 ACTION_DURATION = 100;

    if (m_flags.offBit(eFlags::Landing)) {
        calcRotation();
        calcLanding();
    }

    bool actionEnded = m_frame >= ACTION_DURATION;

    if (actionEnded) {
        if (m_flags.offBit(eFlags::Landing)) {
            physics()->composeDecayingExtraRot(m_rotation);
        }
    } else if (m_flags.offBit(eFlags::Landing)) {
        m_rotation.setAxisRotation(DEG2RAD * m_yawAngle, m_rotAxis);
        physics()->composeExtraRot(m_rotation);
    }

    return actionEnded;
}

/// @addr{0x80568D34}
bool KartAction::calcActionAwayFlipTwice() {
    constexpr u32 ACTION_DURATION = 140;

    auto &status = state()->status();
    if (status.onBit(eStatus::GroundStart)) {
        if (m_groundStartLaunchTimer++ == 0) {
            EGG::Vector3f extVel = dynamics()->extVel();
            extVel.y = 25.0f;
            dynamics()->setExtVel(extVel);
        }
    }

    if (m_flags.offBit(eFlags::Landing)) {
        calcRotation();
        calcLanding();
    }

    bool actionEnded = m_frame >= ACTION_DURATION;

    if (actionEnded) {
        if (m_flags.offBit(eFlags::Landing)) {
            physics()->composeDecayingExtraRot(m_rotation);
        }
    } else if (m_flags.offBit(eFlags::Landing)) {
        m_rotation.setAxisRotation(DEG2RAD * m_yawAngle, m_rotAxis);
        physics()->composeExtraRot(m_rotation);
    }

    return actionEnded;
}

/// @addr{0x805692B4}
bool KartAction::calcLargeFlipAction() {
    constexpr f32 PITCH_DECAY = 0.971f;
    constexpr f32 TOTAL_DELTA_PITCH = 720.0f;
    constexpr f32 PHASE_DELTA = 4.0f;
    constexpr f32 WOBBLE_AMPLITUDE = 18.1f;
    constexpr f32 BOUNCE_FACTOR = 5.0f;

    bool decayingRot = false;
    bool stuntRot = false;

    if (m_flags.onBit(eFlags::FlipBounce)) {
        ++m_flipBounceFrames;
    } else {
        if (m_pitchTraveled < TOTAL_DELTA_PITCH) {
            m_pitchTraveled += m_pitchRotVel;
            m_pitch -= m_pitchRotVel;
            m_pitchRotVel *= (m_pitchRotVel > 1.0f) ? PITCH_DECAY : 1.0f;
        } else {
            m_pitch = 0.0f;
        }

        f32 sin;

        if (EGG::Mathf::abs(m_wobblePhase) < 360.0f) {
            m_wobblePhase += PHASE_DELTA;
            sin = EGG::Mathf::SinFIdx(DEG2FIDX * m_wobblePhase);
        } else {
            sin = 0.0f;
        }

        EGG::Matrix34f mat;
        mat.setAxisRotation(DEG2RAD * (WOBBLE_AMPLITUDE * sin), EGG::Vector3f::ez);
        m_rotation.setAxisRotation(DEG2RAD * m_pitch, mat.ps_multVector(EGG::Vector3f::ex));

        stuntRot = true;
    }

    bool actionEnded = false;
    auto &status = KartObjectProxy::status();
    bool touchingGround = status.onBit(eStatus::TouchingGround);

    if (m_flags.offBit(eFlags::LandingFromFlip) && touchingGround && move()->up().y > 0.0f &&
            m_frame > 50) {
        m_flags.setBit(eFlags::LandingFromFlip);
        dynamics()->setExtVel(move()->up().proj(EGG::Vector3f::ey) * BOUNCE_FACTOR);
    }

    if ((m_currentAction != Action::HighLaunchLoseItem && m_frame < 10) || !touchingGround) {
        dynamics()->setExtVel(EGG::Vector3f(0.0f, dynamics()->extVel().y, 0.0f));
    }

    if ((touchingGround && move()->up().dot(EGG::Vector3f::ey) > 0.0f) || m_frame >= 300) {
        if (m_frame >= 40) {
            status.resetBit(eStatus::LargeFlipHit);
        }

        if (m_frame <= 120) {
            if (m_flags.onBit(eFlags::FlipBounce)) {
                if (m_flipBounceFrames > 30) {
                    actionEnded = true;
                }
            } else {
                if (m_frame >= 40 && m_frame <= 80) {
                    decayingRot = true;
                    m_flags.setBit(eFlags::FlipBounce);
                    status.resetBit(eStatus::LargeFlipHit);
                }
            }
        } else {
            actionEnded = true;
        }
    }

    if (decayingRot) {
        physics()->composeDecayingStuntRot(m_rotation);
    } else if (stuntRot) {
        physics()->composeStuntRot(m_rotation);
    }

    return actionEnded;
}

/// @addr{0x80569A1C}
bool KartAction::calcPressAction() {
    EGG::Vector3f extVel = KartObjectProxy::extVel();
    extVel.y = std::min(0.0f, extVel.y);
    dynamics()->setExtVel(extVel);

    return m_frame > m_crushActionDuration;
}

/* ================================ *
 *     END FUNCTIONS
 * ================================ */

/// @addr{0x8056837C}
void KartAction::endSpin(bool arg) {
    if (arg) {
        physics()->composeDecayingExtraRot(m_rotation);
    }
}

/// @addr{0x80568C7C} @addr{0x805686DC} @addr{0x80568F68}
void KartAction::endLaunchAction(bool arg) {
    if (arg) {
        physics()->composeDecayingExtraRot(m_rotation);
    }
}

} // namespace Kinoko::Kart
