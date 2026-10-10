#include "KartJump.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartPhysics.hh"

namespace Kinoko::Kart {

/// @addr{0x805764FC}
/// @brief Calculates the rotation of the kart during a trick
/// @details @ref m_rotSpeed decays by @ref m_decayRate every frame, clamped to a minimum value
/// specified by @ref TrickProperties::minRotSpeed. Subtracts @ref m_decayRateDelta from @ref
/// m_decayRate each frame, clamping it to a minimum value of @ref TrickProperties::minDecayRate.
/// Increments m_angle by @ref m_rotSpeed every frame, clamping it to a maximum value of @ref
/// m_targetAngle. Finally, applies the computed @ref m_rot to the kart via @ref
/// KartPhysics::composeStuntRot().
void KartJump::calcRot() {
    m_rotSpeed = std::max(m_rotSpeed * m_decayRate, m_properties.minRotSpeed);
    m_decayRate = std::max(m_decayRate - m_decayRateDelta, m_properties.minDecayRate);
    m_angle = std::min(m_angle + m_rotSpeed, m_targetAngle);

    switch (m_type) {
    case TrickType::KartFlipTrickZ:
        m_rot.setRPY(0.0f, 0.0f, -(m_angle * DEG2RAD) * m_rotSign);
        break;
    case TrickType::FlipTrickYLeft:
    case TrickType::FlipTrickYRight:
        m_rot.setRPY(0.0f, m_angle * DEG2RAD * m_rotSign, 0.0f);
        break;
    default:
        break;
    }

    physics()->composeStuntRot(m_rot);
}

/// @addr{0x80576460}
/// @brief Assigns the trick properties and target angle based on the trick type and surface variant
/// @details Indexes into a static properties table based on the trick @ref m_variant to set both
/// @ref m_properties and @ref m_targetAngle. @ref m_rotSpeed is initialized based on @ref
/// TrickProperties::initRotSpeed and @ref m_decayRateDelta is initialized based on @ref
/// TrickProperties::initDecayRateDelta. Finally, resets @ref m_angle to zero, @ref m_decayRate to
/// `1.0f`, and @ref m_rot to the identity quaternion.
void KartJump::setupProperties() {
    static constexpr std::array<TrickProperties, 3> TRICK_PROPERTIES = {{
            {11.0f, 1.5f, 0.9f, 0.0018f},
            {14.0f, 1.5f, 0.9f, 0.0006f},
            {7.5f, 2.5f, 0.93f, 0.05f},
    }};

    static constexpr std::array<f32, 3> FINAL_ANGLES = {{
            360.0f,
            720.0f,
            180.0f,
    }};

    if (m_variant == TrickVariant::SingleFlipTrick) {
        m_properties = TRICK_PROPERTIES[0];
        m_targetAngle = FINAL_ANGLES[0];
    } else if (m_variant == TrickVariant::DoubleFlipTrick) {
        m_properties = TRICK_PROPERTIES[1];
        m_targetAngle = FINAL_ANGLES[1];
    } else if (m_type == TrickType::BikeSideStuntTrick) {
        m_properties = TRICK_PROPERTIES[2];
        m_targetAngle = FINAL_ANGLES[2];
    }

    m_rotSpeed = m_properties.initRotSpeed;
    m_decayRateDelta = m_properties.initDecayRateDelta;
    m_angle = 0.0f;
    m_decayRate = 1.0f;
    m_rot = EGG::Quatf::ident;
}

/// @addr{0x80575D7C}
/// @brief Starts a trick if the player is moving fast enough and the trick start flag is set
/// @param left The left vector of the kart at the start of the trick
/// @details If the @ref eStatus::TrickStart bit is not set in the kart's @ref Status, early returns
/// without starting a trick. If the kart is moving at more than 50% of its base speed, maps the
/// @ref KartState::m_boostRampType to a trick variant, and then calls @ref start() to initialize
/// the trick. Regardless, clears the @ref eStatus::TrickStart flag.
void KartJump::tryStart(const EGG::Vector3f &left) {
    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::TrickStart)) {
        return;
    }

    if (m_move->speedRatioCapped() > 0.5f) {
        s32 boostRampType = state()->boostRampType();

        if (boostRampType == 0) {
            m_variant = TrickVariant::DoubleFlipTrick;
        } else if (boostRampType == 1) {
            m_variant = TrickVariant::SingleFlipTrick;
        } else {
            m_variant = TrickVariant::StuntTrick;
        }

        start(left);
    }

    status.resetBit(eStatus::TrickStart);
}

/// @addr{0x805763E4}
/// @brief Handles trick inputs and rotation every frame
/// @details Decrements @ref m_trickDelay every frame. Checks if we need to update the kart's
/// rotation due to a trick (indicated by @ref eStatus::TrickRot in the kart's @ref Status), and
/// checks if the player is attempting to start a trick via @ref calcInput().
void KartJump::calc() {
    m_trickDelay = std::max(0, m_trickDelay - 1);

    if (status().onBit(eStatus::TrickRot)) {
        calcRot();
    }

    calcInput();
}

/// @addr{0x80575B38}
/// @brief Checks player controller input to try starting a trick
/// @details If trick inputs should not be ignored (determined via @ref shouldIgnoreTrick() and the
/// user is pressing a trick input, sets @ref m_nextTrick to the current trick input and sets @ref
/// m_leniencyTimer to `14`. If the kart has more than 2 frames of airtime, sets the @ref
/// eStatus::TrickStart flag. Decrements @ref m_leniency if a trick cannot be performed (no airtime,
/// more than 10 frames of airtime, not on a trickable surface, or @ref shouldIgnoreTrick() returns
/// `true`). Finally, if the kart is on the ground (indicated via the @ref eStatus::TouchingGround
/// bit in the kart's @ref Status) and not on a boost ramp, resets @ref m_boostRampTrick to `false`.
void KartJump::calcInput() {
    constexpr s16 TRICK_ALLOW_TIMER = 14;

    System::Trick trick = inputs()->currentState().trick;

    if (!shouldIgnoreTrick() && trick != System::Trick::None) {
        m_nextTrick = trick;
        m_leniencyTimer = TRICK_ALLOW_TIMER;
    }

    auto &status = KartObjectProxy::status();

    u32 airtime = state()->airtime();
    if (airtime == 0 || m_leniencyTimer < 1 || airtime > 10 ||
            (status.offBit(eStatus::Trickable) && state()->boostRampType() < 0) ||
            shouldIgnoreTrick()) {
        m_leniencyTimer = std::max(0, m_leniencyTimer - 1);
    } else {
        if (airtime > 2) {
            status.setBit(eStatus::TrickStart);
        }
        if (status.onBit(eStatus::RampBoost)) {
            m_boostRampTrick = true;
        }
    }
    if (status.onBit(eStatus::TouchingGround) &&
            collide()->surfaceFlags().offBit(KartCollide::eSurfaceFlags::BoostRamp)) {
        m_boostRampTrick = false;
    }
}

/// @addr{0x805766B8}
/// @brief Called when a trick has ended
/// @details If the kart is currently in a trick rotation (indicated by @ref eStatus::TrickRot),
/// applies a decaying rotation to the kart. Resets the @ref eStatus::InATrick and @ref
/// eStatus::TrickRot flags in the kart's @ref Status. Finally, resets @ref m_boostRampTrick to
/// `false`.
void KartJump::end() {
    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::TrickRot)) {
        physics()->composeDecayingStuntRot(m_rot);
    }

    status.resetBit(eStatus::InATrick, eStatus::TrickRot);
    m_boostRampTrick = false;
}

/// @addr{0x80576230}
/// @brief Influences the kart's pitch and direction when starting a trick
/// @details If the kart is on a jump pad (indicated by the @ref eStatus::JumpPad flag in the kart's
/// @ref Status), the function returns early. The max pitch angle from horizontal and the pitch
/// correction applied are mapped based off the vehicle's weight class and the trick variant. If the
/// kart is already pitched above the max angle, no pitch correction is applied.
/// @note In Kinoko, we take creative liberty to move the @ref eStatus::JumpPad check before the
/// angle computation as a slight performance improvement.
void KartJump::setAngle(const EGG::Vector3f &left) {
    static constexpr std::array<std::array<AngleProperties, 3>, 3> ANGLE_PROPERTIES = {{
            {{
                    {40.0f, 15.0f},
                    {45.0f, 20.0f},
                    {45.0f, 20.0f},
            }},
            {{
                    {36.0f, 13.0f},
                    {42.0f, 18.0f},
                    {42.0f, 18.0f},
            }},
            {{
                    {32.0f, 11.0f},
                    {39.0f, 16.0f},
                    {39.0f, 16.0f},
            }},
    }};

    if (status().onBit(eStatus::JumpPad)) {
        return;
    }

    f32 vel1YDot = m_move->intVelDir().dot(EGG::Vector3f::ey);
    EGG::Vector3f vel1YCross = m_move->intVelDir().cross(EGG::Vector3f::ey);
    f32 pitch = EGG::Mathf::abs(EGG::Mathf::atan2(vel1YCross.length(), vel1YDot));
    f32 angle = 90.0f - (pitch * RAD2DEG);
    u32 weightClass = static_cast<u32>(param()->stats().weightClass);
    f32 targetAngle = ANGLE_PROPERTIES[weightClass][static_cast<u32>(m_variant)].maxAngle;

    if (angle > targetAngle) {
        return;
    }

    f32 rotAngle = ANGLE_PROPERTIES[weightClass][static_cast<u32>(m_variant)].rotAngle;

    if (angle + rotAngle > targetAngle) {
        rotAngle = targetAngle - angle;
    }

    EGG::Matrix34f nextDir;
    nextDir.setAxisRotation(-rotAngle * DEG2RAD, left);
    m_move->setDir(nextDir.ps_multVector(m_move->dir()));
    m_move->setVel1Dir(m_move->dir());
}

/// @addr{0x80575EE8}
/// @brief Called when a trick has started
/// @param left The left vector of the kart at the start of the trick
/// @details Initializes the trick type, rotation direction, and trick properties via a call to @ref
/// init(). Influences the kart's rotation and orientation via a call to @ref setAngle. Sets the
/// @ref eStatus::InATrick bit in the kart's @ref Status. Sets @ref m_trickDelay to `5`.
void KartJump::start(const EGG::Vector3f &left) {
    init();
    setAngle(left);
    status().setBit(eStatus::InATrick);
    m_trickDelay = 5;
}

/// @addr{0x8057616C}
/// @brief Initializes the trick type, rotation direction, and trick properties based on the
/// trick variant
/// @details If the trick @ref m_variant is @ref TrickVariant::StuntTrick, then simply sets @ref
/// m_type to @ref TrickType::StuntTrickBasic and returns. Otherwise, @ref m_type and @ref m_rotSign
/// are set based on the player's input: up and down tricks have @ref m_type set to @ref
/// TrickType::KartFlipTrickZ while left/right inputs are mapped to the corresponding left/right
/// flip trick types. @ref m_rotSign is set based on the direction of the trick: up and right tricks
/// have a negative rotation sign, while down and left tricks have a positive rotation sign.
void KartJump::init() {
    if (m_variant == TrickVariant::StuntTrick) {
        m_type = TrickType::StuntTrickBasic;
        return;
    }

    if (m_nextTrick < System::Trick::Left) {
        m_type = TrickType::KartFlipTrickZ;
        m_rotSign = (m_nextTrick == System::Trick::Up) ? -1.0f : 1.0f;
    } else {
        m_type = static_cast<TrickType>(m_nextTrick);
        m_rotSign = (m_type == TrickType::FlipTrickYRight) ? -1.0f : 1.0f;
    }

    setupProperties();
    status().setBit(eStatus::TrickRot);
}

/// @addr{0x80576994}
/// @brief Calculates the rotation of the bike during a trick
/// @details Decays @ref m_rotSpeed by @ref m_decayRate each frame, clamping it to a minimum value
/// of @ref TrickProperties::minRotSpeed. Subtracts @ref m_decayRateDelta from @ref m_decayRate each
/// frame, clamping it to a minimum value of @ref TrickProperties::minDecayRate. Increments m_angle
/// by @ref m_rotSpeed every frame, clamping it to a maximum value of @ref m_targetAngle.  Finally,
/// applies the computed @ref m_rot to the kart via @ref KartPhysics::composeStuntRot().
void KartJumpBike::calcRot() {
    /// @brief Computed using double precision, so we hard-code it.
    constexpr f32 PI_OVER_9 = 0.34906584f;
    /// @brief Computed using double precision, so we hard-code it.
    constexpr f32 PI_OVER_3 = 1.0471976f;

    m_rotSpeed = std::max(m_rotSpeed * m_decayRate, m_properties.minRotSpeed);
    m_decayRate = std::max(m_decayRate - m_decayRateDelta, m_properties.minDecayRate);
    m_angle = std::min(m_angle + m_rotSpeed, m_targetAngle);

    switch (m_type) {
    case TrickType::BikeFlipTrickNose:
    case TrickType::BikeFlipTrickTail:
        m_rot.setRPY(-(m_angle * DEG2RAD) * m_rotSign, 0.0f, 0.0f);
        break;
    case TrickType::FlipTrickYLeft:
    case TrickType::FlipTrickYRight:
        m_rot.setRPY(0.0f, m_angle * DEG2RAD * m_rotSign, 0.0f);
        break;
    case TrickType::BikeSideStuntTrick: {
        f32 sin = EGG::Mathf::SinFIdx(m_angle * DEG2FIDX);
        m_rot.setRPY(sin * -PI_OVER_9, (sin * m_rotSign) * -PI_OVER_3,
                (sin * m_rotSign) * PI_OVER_9);
    } break;
    default:
        break;
    }

    physics()->composeStuntRot(m_rot);
}

/// @addr{0x80576758}
/// @copydoc KartJump::start()
/// Also cancels wheelies via @ref KartMoveBike::cancelWheelie().
void KartJumpBike::start(const EGG::Vector3f &left) {
    KartJump::start(left);

    KartMoveBike *moveBike = static_cast<KartMoveBike *>(m_move);
    moveBike->cancelWheelie();
}

/// @addr{0x8057689C}
/// @copybrief KartJump::init()
/// @details If @ref m_variant is a @ref TrickVariant::StuntTrick and @ref m_nextTrick is an up or
/// down @ref System::Trick, then this function early returns (meaning @ref m_nextTrick will default
/// to @ref TrickType::StuntTrickBasic). For left/right stunt tricks, sets @ref m_type to @ref
/// TrickType::BikeSideStuntTrick, sets @ref m_rotSign depending on the direction of the trick
/// input, calls @ref setupProperties() to assign the relevant trick properties, and sets @ref
/// m_targetAngle to `180.0f`. For other trick variants, sets @ref m_type based on @ref m_nextTrick,
/// determines @ref m_rotSign depending on the trick type, and calls @ref setupProperties().
/// Regardless of the variant, sets the @ref eStatus::TrickRot bit in the kart's @ref Status.
void KartJumpBike::init() {
    constexpr f32 STUNT_TRICK_TARGET_ANGLE = 180.0f;

    if (m_variant == TrickVariant::StuntTrick) {
        if (m_nextTrick < System::Trick::Left) {
            return;
        }

        m_type = TrickType::BikeSideStuntTrick;
        m_rotSign = (m_nextTrick == System::Trick::Right) ? -1.0f : 1.0f;
        setupProperties();
        m_targetAngle = STUNT_TRICK_TARGET_ANGLE;
    } else {
        m_type = static_cast<TrickType>(m_nextTrick);
        m_rotSign =
                (m_type == TrickType::FlipTrickYRight || m_type == TrickType::BikeFlipTrickTail) ?
                -1.0f :
                1.0f;
        setupProperties();
    }

    status().setBit(eStatus::TrickRot);
}

} // namespace Kinoko::Kart
