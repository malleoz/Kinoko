#include "KartScale.hh"

namespace Kinoko::Kart {

/// @addr{0x8056AD44}
/// @brief Constructor that initializes the @ref m_scaleTarget array based on the static base values
/// and the kart's shrink and mega scale stats
KartScale::KartScale(const KartParam::Stats &stats) {
    reset();

    for (u16 i = 0; i < m_scaleTarget.size(); ++i) {
        m_scaleTarget[i] = s_baseScaleTarget[i];

        switch (i) {
        case 1:
            m_scaleTarget[i] = stats.shrinkScale;
            break;
        case 2:
            m_scaleTarget[i] = stats.megaScale;
            break;
        default:
            break;
        }
    }
}

/// @addr{0x8056B5A8}
/// @brief Default destructor
KartScale::~KartScale() = default;

/// @addr{0x8056AF10}
/// @brief Resets the kart's scale-related properties to their default values
void KartScale::reset() {
    m_shrinkType = ThunderScaleType::None;
    m_shrinkScale = EGG::Vector3f::unit;
    m_shrinkOffset.setZero();
    m_shrinkRate.setZero();
    m_calcShrink = false;
    m_shrinkAnmFrame = 0.0f;
    m_crushState = CrushState::None;
    m_calcCrush = false;
    m_uncrushAnmFrame = 0.0f;
    m_pressScale = EGG::Vector3f::unit;
}

/// @addr{0x8056B218}
/// @brief Every frame, calculates the kart's current shrink and crush scales
void KartScale::calc() {
    if (m_calcShrink) {
        const Abstract::g3d::ResAnmChr *scaleAnm;
        if (m_shrinkType == ThunderScaleType::ScaleUp) {
            scaleAnm = KartObjectManager::ThunderScaleUpAnmChr();
        } else if (m_shrinkType == ThunderScaleType::ScaleDown) {
            scaleAnm = KartObjectManager::ThunderScaleDownAnmChr();
        } else {
            PANIC("Invalid scale type");
        }
        ASSERT(scaleAnm);

        auto anmResult = scaleAnm->getAnmResult(m_shrinkAnmFrame, 0);
        m_shrinkScale = m_shrinkOffset + m_shrinkRate * anmResult.scale();

        m_shrinkAnmFrame += 1.0f;
        if (m_shrinkAnmFrame > scaleAnm->frameCount()) {
            m_calcShrink = false;
            m_shrinkScale.set(m_scaleTarget[static_cast<size_t>(m_shrinkType)]);
            m_shrinkOffset.setZero();
            m_shrinkRate.setZero();
        }
    }

    calcCrush();
}

/// @addr{0x8056AFB4}
/// @brief Initiates the kart's shrink or Mega Mushroom grow animation
/// @param inMega Indicates whether the shrink/grow animation is due to a Mega Mushroom
/// @details Also calculates the necessary rate and offset to map the shrink animation's
/// progress to the kart's real world scale.
void KartScale::startShrink(bool inMega) {
    m_shrinkType = inMega ? ThunderScaleType::ScaleMegaUp : ThunderScaleType::ScaleDown;
    m_shrinkAnmFrame = 0.0f;
    m_calcShrink = true;
    size_t typeIdx = static_cast<size_t>(m_shrinkType);
    f32 scale = m_scaleTarget[typeIdx];
    m_shrinkRate = (EGG::Vector3f(scale, scale, scale) - m_shrinkScale) /
            (s_baseScaleTarget[typeIdx] - s_baseScaleStart[typeIdx]);
    m_shrinkOffset = m_shrinkScale - m_shrinkRate * s_baseScaleStart[typeIdx];
}

/// @addr{0x8056B168}
/// @brief Ends the kart's shrink animation, returning it to its normal scale
/// @param inMega Indicates whether the shrink animation is due to a Mega Mushroom
/// @details Also calculates the necessary rate and offset to map the shrink animation's
/// progress to the kart's real world scale.
void KartScale::endShrink(bool inMega) {
    m_shrinkType = inMega ? ThunderScaleType::ScaleMegaDown : ThunderScaleType::ScaleUp;
    m_shrinkAnmFrame = 0.0f;
    m_calcShrink = true;
    size_t typeIdx = static_cast<size_t>(m_shrinkType);
    f32 tmp = m_scaleTarget[typeIdx];
    m_shrinkRate = (EGG::Vector3f(tmp, tmp, tmp) - m_shrinkScale) /
            (s_baseScaleTarget[typeIdx] - s_baseScaleStart[typeIdx]);
    m_shrinkOffset = m_shrinkScale - m_shrinkRate * s_baseScaleStart[typeIdx];
}

/// @addr{0x8056B45C}
/// @brief Calculates the kart's current crush animation scale based on its crush state
/// @details When the kart gets crushed, this function decreases its Y-scale until it reaches the
/// fully squished scale (0.2f). When the kart is uncrushed, it interpolates the scale back toward
/// its normal value based on the uncrush animation frame.
void KartScale::calcCrush() {
    constexpr f32 SCALE_SPEED = 0.2f;

    if (!m_calcCrush || m_crushState == CrushState::None) {
        return;
    }

    if (m_crushState == CrushState::Crush) {
        m_pressScale.y -= SCALE_SPEED;
        if (m_pressScale.y < CRUSH_SCALE) {
            m_pressScale.y = CRUSH_SCALE;
            m_calcCrush = false;
        }
    } else {
        m_pressScale = GetPressAnmScale(m_uncrushAnmFrame);

        const auto *scaleAnm = KartObjectManager::PressScaleUpAnmChr();
        ASSERT(scaleAnm);

        if (++m_uncrushAnmFrame > static_cast<f32>(scaleAnm->frameCount())) {
            m_calcCrush = false;
        }
    }
}

} // namespace Kinoko::Kart
