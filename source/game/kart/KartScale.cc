#include "KartScale.hh"

#include "game/kart/KartObjectManager.hh"

namespace Kart {

/// @addr{0x8056AD44}
KartScale::KartScale() {
    // We will need m_60, m_64,
    reset();
}

/// @addr{0x8056B5A8}
KartScale::~KartScale() = default;

/// @addr{0x8056AF10}
void KartScale::reset() {
    m_60 = -1;
    m_64 = false;
    m_scaleAnmFrame = 0.0f;
    m_targetScale = EGG::Vector3f(1.0f, 1.0f, 1.0f);
}

/// @addr{0x8056B218}
// Not needed until the unsquish
void KartScale::calc() {
    if (m_64 && m_60 != -1) {
        if (m_60 == 0) {
            m_targetScale.y -= 0.2f;
            if (m_targetScale.y < 0.3f) {
                m_targetScale.y = 0.3f;
                m_64 = false;
            }
        } else {
            m_targetScale = getAnmScale(m_scaleAnmFrame);

            const auto &scaleAnm = KartObjectManager::Instance()->RaceScaleAnmChr();
            if (++m_scaleAnmFrame > static_cast<f32>(scaleAnm.frameCount())) {
                m_64 = false;
            }
        }
    }
}

/// @addr{0x8056B060}
void KartScale::resetScale() {
    m_60 = 0;
    m_targetScale = EGG::Vector3f(1.0f, 1.0f, 1.0f);
    m_scaleAnmFrame = 0.0f;
    m_64 = true;
}

/// @addr{0x8056B094}
void KartScale::calcCrushAnimation() {
    m_60 = 1;
    m_targetScale = EGG::Vector3f(1.0f, 0.3f, 1.0f);
    m_scaleAnmFrame = 0.0f;
    m_64 = true;
}

/// @addr{0x8056ACF4}
EGG::Vector3f KartScale::getAnmScale(f32 frame) const {
    const auto &scaleAnm = KartObjectManager::Instance()->RaceScaleAnmChr();
    auto result = scaleAnm.getAnmResult(frame, 0);
    return result.scale();
}

} // namespace Kart
