#include "ObjectFlamePoleV.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @addr{0x806C3AA4}
ObjectFlamePoleV::ObjectFlamePoleV(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), StateManager(this), m_initPosY(m_pos.y) {
    constexpr f32 M_D8 = 384.0f;
    constexpr f32 BIG_MULTIPLIER = 14.0f;

    m_initDelay = static_cast<u32>(params.setting(1));
    m_e4 = static_cast<u32>(params.setting(0));
    m_state4Duration = static_cast<u32>(params.setting(3)) + 200;
    m_scaleFactor = params.setting(2) == 0 ? 5.0f : static_cast<f32>(params.setting(2));

    m_isBig = strcmp(getName(), "FlamePole_v_big") == 0;
    if (m_isBig) {
        m_scaledHeight = BIG_MULTIPLIER * M_D8 * m_scaleFactor;
    } else {
        m_scaledHeight = M_D8 * m_scaleFactor;
    }
}

/// @addr{0x806C47B4}
ObjectFlamePoleV::~ObjectFlamePoleV() = default;

/// @addr{0x806C3E90}
void ObjectFlamePoleV::init() {
    constexpr EGG::Vector3f BIG_SCALE = EGG::Vector3f(13.0f, 14.0f, 13.0f);
    constexpr f32 RADIUS = 70.0f;

    m_nextStateId = 0;

    disableCollision();

    if (m_isBig) {
        m_scale = BIG_SCALE;
        m_flags.setBit(eFlags::Scale);
    } else if (0.0f != m_scaleFactor) {
        m_scale = EGG::Vector3f(m_scaleFactor, m_scaleFactor, m_scaleFactor);
        m_flags.setBit(eFlags::Scale);
    }

    resize(RADIUS * m_scale.y, 0.0f);

    m_10c = 0.0f;
    m_104 = m_scaledHeight / 180.0f;
}

/// @addr{0x806C3FCC}
void ObjectFlamePoleV::calc() {
    if (System::RaceManager::Instance()->timer() <= m_initDelay) {
        return;
    }

    StateManager::calc();

    m_flags.setBit(eFlags::Position);
    m_pos.y = m_10c + (m_initPosY - m_scaledHeight);
}

/// @addr{0x806C42A0}
void ObjectFlamePoleV::calcState2() {
    if (static_cast<f32>(m_currentFrame) >=
            static_cast<f32>(m_e4) - 50.0f - 60.0f - 180.0f - static_cast<f32>(m_state4Duration)) {
        m_nextStateId = 3;
    }

    m_10c = m_scaledHeight +
            50.0f *
                    EGG::Mathf::SinFIdx(
                            DEG2FIDX * (360.0f * static_cast<f32>(m_currentFrame) / 30.0f));
}

/// @addr{0x806C43E8}
void ObjectFlamePoleV::calcState3() {
    if (!m_isBig) {
        if (m_10c <= 0.0f) {
            m_nextStateId = 4;
        }
    } else if (m_10c <= -300.0f) {
        m_nextStateId = 4;
    }

    m_10c = m_110 - m_104 * static_cast<f32>(m_currentFrame);
}

}; // namespace Field
