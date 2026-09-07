#include "KartModel.hh"

#include "game/kart/KartMove.hh"

#include "game/system/RaceConfig.hh"

namespace Kinoko::Render {

/// @brief Constructor
KartModel::KartModel() {
    m_isLeaningLeft = false;
    m_isLeaningRight = false;
    m_leanWobble = 0.0f;
    m_leanAmplitude = 1.0f;
    m_bodyLeanAngle = 0.0f;
    m_bodyLeanStep = 0.0f;
    m_prevBurnoutPitch = 0.0f;
}

/// @brief Default virtual destructor
KartModel::~KartModel() = default;

/// @addr{0x807CB198}
/// @brief Calculates the leaning state of the kart based on input and drift status
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
/// @details This is a virtual function in the basegame, but since it is not overridden by any
/// derived class, we can devirtualize in Kinoko.
void KartModel::calcLeanAngle() {
    const auto &status = KartObjectProxy::status();

    if (status.onBit(Kart::eStatus::Burnout)) {
        m_leanAmplitude = 1.0f;

        f32 pitch = move()->burnout().pitch();
        f32 fVar2 = pitch + 75.0f * (pitch - m_prevBurnoutPitch);
        f32 fVar4 = std::min(0.2f, 0.04f * EGG::Mathf::abs(fVar2));
        fVar4 = fVar2 > 0.0f ? fVar4 : -fVar4;

        m_prevBurnoutPitch = pitch;
        m_leanWobble += fVar4;
    } else {
        m_prevBurnoutPitch = 0.0f;
        m_leanWobble *= 0.9f;
    }

    bool frozenInIce =
            System::RaceConfig::Instance()->raceScenario().course == Course::N64_Sherbet_Land &&
            status.onBit(Kart::eStatus::InRespawn, Kart::eStatus::AfterRespawn);
    f32 xStick = frozenInIce ? 0.0f : inputs()->currentState().stick.x;
    bool isInCannon = status.onBit(Kart::eStatus::InCannon);
    f32 fVar2 = isInCannon ? 0.02f : 0.1f;

    f32 local_f31 = m_leanWobble;
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

    if (local_f31 < -m_leanAmplitude || m_leanAmplitude < local_f31) {
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

    f32 dVar12 = 0.0f;
    m_bodyLeanStep = dVar13 * 0.1f;

    if (isBike()) {
        dVar12 = -m_leanWobble * dVar13;

        if (m_isLeaningLeft) {
            dVar12 += m_isInsideDrift ? 5.0f : 10.0f;
        } else if (m_isLeaningRight) {
            dVar12 -= m_isInsideDrift ? 5.0f : 10.0f;
        }
    } else {
        if (!m_isLeaningLeft && m_isLeaningRight) {
            dVar12 -= 5.0f;
        } else {
            dVar12 += 5.0f;
        }
    }

    if (dVar12 <= m_bodyLeanAngle) {
        m_bodyLeanAngle -= m_bodyLeanStep;
        m_bodyLeanAngle = std::max(m_bodyLeanAngle, dVar12);
    } else {
        m_bodyLeanAngle += m_bodyLeanStep;
        m_bodyLeanAngle = std::min(m_bodyLeanAngle, dVar12);
    }

    body()->setLeanAngle(m_bodyLeanAngle);
}

} // namespace Kinoko::Render
