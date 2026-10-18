#include "KartModel.hh"

#include "game/kart/KartBody.hh"
#include "game/kart/KartMove.hh"
#include "game/kart/KartState.hh"

#include "game/system/KPadController.hh"
#include "game/system/RaceConfig.hh"

namespace Kinoko::Render {

/// @brief Constructor
/// @details Initializes @ref m_isLeaningLeft and @ref m_isLeaningRight to `false`. Initializes
/// @ref m_leanWobble, @ref m_bodyLeanAngle, @ref m_bodyLeanStep, and @ref m_prevBurnoutYaw to
/// zero. Initializes @ref m_leanAmplitude to `1.0f`.
KartModel::KartModel() {
    m_isLeaningLeft = false;
    m_isLeaningRight = false;
    m_leanWobble = 0.0f;
    m_leanAmplitude = 1.0f;
    m_bodyLeanAngle = 0.0f;
    m_bodyLeanStep = 0.0f;
    m_prevBurnoutYaw = 0.0f;
}

/// @brief Default virtual destructor
KartModel::~KartModel() = default;

/// @addr{0x807C8758}
/// @brief Initializes the kart model with the given player index and vehicle type
/// @details Calls @ref initModel() with the player index and vehicle type. Also sets @ref
/// m_prevBurnoutYaw to `0.0f`.
void KartModel::init() {
    initModel(param()->playerIdx(), isBike());
    m_prevBurnoutYaw = 0.0f;
}

/// @addr{0x807CB360}
/// @brief Every frame, updates the kart model
/// @details In Kinoko, we omit a lot of implementation since we only care about the kart's
/// leaning behavior in @ref calcLean().
void KartModel::calc() {
    calcLean();
}

/// @addr{0x807CB198}
/// @brief Calculates the leaning state of the kart based on input and drift status
/// @details This function only has an effect if the kart is currently drifting or the player is
/// turning while in a stand-still mini-turbo (SSMT). If the player is turning while in a SSMT, then
/// @ref m_isLeaningLeft or @ref m_isLeaningRight will be set depending on the direction of the
/// turn. Otherwise if the kart is drifting, then @ref m_isLeaningLeft or @ref m_isLeaningRight will
/// be set based on the drift direction.
void KartModel::calcLeaning() {
    m_isLeaningRight = false;
    m_isLeaningLeft = false;
    auto &status = KartObjectProxy::status();

    bool turnInput = status.onBit(Kart::eStatus::StickLeft, Kart::eStatus::StickRight);
    if (state()->isDrifting() || (status.onBit(Kart::eStatus::ChargingSSMT) && turnInput)) {
        if (hopStickX() == 1) {
            m_isLeaningLeft = true;
        } else {
            if (hopStickX() == -1) {
                m_isLeaningRight = true;
            } else if (status.offBit(Kart::eStatus::StickLeft)) {
                m_isLeaningRight = true;
            } else {
                m_isLeaningLeft = true;
            }
        }
    }
}

/// @addr{0x807CD32C}
/// @brief Calculates the vehicle's lean wobble and angle and applies it to @ref Kart::KartBody
/// @details If the kart is in a burnout, then @ref m_leanWobble increments based on the change in
/// burnout rotation (@ref Kart::KartBurnout::m_yaw) from the previous frame. Otherwise if the kart
/// is not in a burnout, then @ref m_leanWobble decays by 10% each frame. Afterwards, @ref
/// m_leanWobble is nudged based on the stick input, unless the kart is frozen in ice while
/// respawning on N64 Sherbet Land. @ref m_leanAmplitude is similarly updated towards the stick
/// input, except its blend rate is dampened if the player is in a cannon. @ref m_leanWobble is then
/// gradually nudged to stay within `[-m_leanAmplitude, m_leanAmplitude]`. Calculates @ref
/// m_bodyLeanAngle by moving it towards a target lean angle by at most @ref m_bodyLeanStep per
/// frame. The target angle is based on `-m_leanWobble` scaled based on the vehicle type (5 degrees
/// for inside drifting bikes, 20 degrees for outside drifting bikes, 15 degrees for karts, and 15
/// degrees when not drifting). A further nudge is applied if actively leaning left or right (5
/// degrees for inside drifting bikes, 10 degrees for all other bikes, and 5 degrees for karts).
/// Finally, applies this result to the kart's body via @ref Kart::KartBody::setLeanAngle().
/// @note This is a virtual function in the basegame, but since it is not overridden by any
/// derived class, we can devirtualize in Kinoko.
void KartModel::calcLeanAngle() {
    const auto &status = KartObjectProxy::status();

    if (status.onBit(Kart::eStatus::Burnout)) {
        m_leanAmplitude = 1.0f;

        f32 yaw = move()->burnout().yaw();
        f32 fVar2 = yaw + 75.0f * (yaw - m_prevBurnoutYaw);
        f32 wobbleDelta = std::min(0.2f, 0.04f * EGG::Mathf::abs(fVar2));
        wobbleDelta = fVar2 > 0.0f ? wobbleDelta : -wobbleDelta;

        m_prevBurnoutYaw = yaw;
        m_leanWobble += wobbleDelta;
    } else {
        m_prevBurnoutYaw = 0.0f;
        m_leanWobble *= 0.9f;
    }

    bool frozenInIce =
            System::RaceConfig::Instance()->raceScenario().course == Course::N64_Sherbet_Land &&
            status.onBit(Kart::eStatus::InRespawn, Kart::eStatus::AfterRespawn);
    f32 xStick = frozenInIce ? 0.0f : inputs()->currentState().stick.x;
    bool isInCannon = status.onBit(Kart::eStatus::InCannon);
    f32 fVar2 = isInCannon ? 0.02f : 0.1f;

    f32 prevLeanWobble = m_leanWobble;
    if (xStick <= 0.2f) {
        if (xStick < -0.2f) {
            m_leanWobble -= fVar2;
        }
    } else {
        m_leanWobble += fVar2;
    }

    xStick = EGG::Mathf::abs(xStick);

    if (isInCannon) {
        xStick *= 0.8f;
        fVar2 = 0.05f;
    }

    m_leanAmplitude += fVar2 * (xStick - m_leanAmplitude);

    if (prevLeanWobble < -m_leanAmplitude || m_leanAmplitude < prevLeanWobble) {
        if (-m_leanAmplitude <= m_leanWobble) {
            if (m_leanAmplitude < m_leanWobble) {
                m_leanWobble -= 0.1f;
            }
        } else {
            m_leanWobble += 0.1f;
        }
    } else if (-m_leanAmplitude <= m_leanWobble) {
        m_leanWobble = std::min(m_leanAmplitude, m_leanWobble);
    } else {
        m_leanWobble = -m_leanAmplitude;
    }

    f32 dVar13 = m_leanWobble;

    if (isBike()) {
        if (state()->isDrifting()) {
            dVar13 = m_isInsideDrift ? 5.0f : 20.0f;
        } else {
            dVar13 = 15.0f;
        }
    } else {
        dVar13 = 15.0f;
    }

    f32 targetBodyLeanAngle = 0.0f;
    m_bodyLeanStep = dVar13 * 0.1f;

    if (isBike()) {
        targetBodyLeanAngle = -m_leanWobble * dVar13;

        if (m_isLeaningLeft) {
            targetBodyLeanAngle += m_isInsideDrift ? 5.0f : 10.0f;
        } else if (m_isLeaningRight) {
            targetBodyLeanAngle -= m_isInsideDrift ? 5.0f : 10.0f;
        }
    } else {
        if (!m_isLeaningLeft && m_isLeaningRight) {
            targetBodyLeanAngle -= 5.0f;
        } else {
            targetBodyLeanAngle += 5.0f;
        }
    }

    if (targetBodyLeanAngle <= m_bodyLeanAngle) {
        m_bodyLeanAngle = std::max(m_bodyLeanAngle - m_bodyLeanStep, targetBodyLeanAngle);
    } else {
        m_bodyLeanAngle = std::min(m_bodyLeanAngle + m_bodyLeanStep, targetBodyLeanAngle);
    }

    body()->setLeanAngle(m_bodyLeanAngle);
}

/// @addr{0x807CB530}
/// @brief Dispatches to two helper functions to update the kart's leaning angle
/// @details Calls @ref calcLeaning() and @ref calcLeanAngle().
void KartModel::calcLean() {
    calcLeaning();
    calcLeanAngle();
}

/// @addr{0x807C7828}
/// @brief Initializes the kart model's internal state based on the player index and vehicle
/// @details For Kinoko, this just initializes @ref m_isInsideDrift.
void KartModel::initModel(u8 /*playerIdx*/, bool /*isBike*/) {
    m_isInsideDrift = vehicleType() == Kart::KartParam::Stats::DriftType::Inside_Drift_Bike;
}

} // namespace Kinoko::Render
