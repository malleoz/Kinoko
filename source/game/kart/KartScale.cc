#include "KartScale.hh"

#include "game/kart/KartObjectManager.hh"

namespace Kart {

/// @addr{0x8056AD44}
KartScale::KartScale() {
    reset();
}

/// @addr{0x8056B5A8}
KartScale::~KartScale() = default;

/// @addr{0x8056AF10}
void KartScale::reset() {
    m_type = -1;
    m_2c = EGG::Vector3f::unit;
    m_14.setZero();
    m_20.setZero();
    m_38 = false;
    m_3c = 0.0f;
    m_crushState = CrushState::None;
    m_calcCrush = false;
    m_uncrushAnmFrame = 0.0f;
    m_currScale = EGG::Vector3f(1.0f, 1.0f, 1.0f);
}

/// @addr{0x8056B218}
void KartScale::calc() {
    if (m_38) {
        const Abstract::g3d::ResAnmChr *scaleAnm;
        if (m_type == 0) {
            scaleAnm = KartObjectManager::ThunderScaleUpAnmChr();
        } else if (m_type == 1) {
            scaleAnm = KartObjectManager::ThunderScaleDownAnmChr();
        } else {
            PANIC("Invalid scale type");
        }
        ASSERT(scaleAnm);

        auto anmResult = scaleAnm->getAnmResult(m_3c, 0);
        m_2c = m_14 + m_20 * anmResult.scale();

        m_3c += 1.0f;
        if (m_3c > scaleAnm->frameCount()) {
            m_38 = false;
            m_2c.set(_808B50C0[m_type]);
            m_14.setZero();
            m_20.setZero();
        }
    }

    calcCrush();
}

/// @addr{0x8056B060}
void KartScale::startCrush() {
    m_crushState = CrushState::Crush;
    m_currScale = EGG::Vector3f(1.0f, 1.0f, 1.0f);
    m_uncrushAnmFrame = 0.0f;
    m_calcCrush = true;
}

/// @addr{0x8056B094}
void KartScale::startUncrush() {
    m_crushState = CrushState::Uncrush;
    m_currScale = EGG::Vector3f(1.0f, CRUSH_SCALE, 1.0f);
    m_uncrushAnmFrame = 0.0f;
    m_calcCrush = true;
}

/// @addr{0x8056AFB4}
void KartScale::FUN_8056AFB4(s32 unk) {
    m_type = unk > 0 ? 2 : 1;
    m_3c = 0.0f;
    m_38 = true;
    f32 tmp = _808B50C0[m_type];
    m_20 = (EGG::Vector3f(tmp, tmp, tmp) - m_2c) / (tmp - _808B50B0[m_type]);
    m_14 = m_2c - m_20 * _808B50B0[m_type];
}

/// @addr{0x8056B168}
void KartScale::FUN_8056B168(s32 unk) {
    m_type = unk > 0 ? 3 : 0;
    m_3c = 0.0f;
    m_38 = true;
    f32 tmp = _808B50C0[m_type];
    m_20 = (EGG::Vector3f(tmp, tmp, tmp) - m_2c) / (tmp - _808B50B0[m_type]);
    m_14 = m_2c - m_20 * _808B50B0[m_type];
}

/// @addr{0x8056B45C}
void KartScale::calcCrush() {
    constexpr f32 SCALE_SPEED = 0.2f;

    if (!m_calcCrush || m_crushState == CrushState::None) {
        return;
    }

    if (m_crushState == CrushState::Crush) {
        m_currScale.y -= SCALE_SPEED;
        if (m_currScale.y < CRUSH_SCALE) {
            m_currScale.y = CRUSH_SCALE;
            m_calcCrush = false;
        }
    } else {
        m_currScale = getAnmScale(m_uncrushAnmFrame);

        const auto *scaleAnm = KartObjectManager::PressScaleUpAnmChr();
        ASSERT(scaleAnm);

        if (++m_uncrushAnmFrame > static_cast<f32>(scaleAnm->frameCount())) {
            m_calcCrush = false;
        }
    }
}

/// @addr{0x8056ACF4}
EGG::Vector3f KartScale::getAnmScale(f32 frame) const {
    const auto *scaleAnm = KartObjectManager::PressScaleUpAnmChr();
    ASSERT(scaleAnm);
    return scaleAnm->getAnmResult(frame, 0).scale();
}

} // namespace Kart
