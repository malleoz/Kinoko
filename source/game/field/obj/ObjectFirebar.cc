#include "ObjectFirebar.hh"

namespace Field {

/// @addr{0x807678F4}
ObjectFirebar::ObjectFirebar(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
    m_numBars = std::max<u32>(1, params.setting(3));
    u32 numFireballs = std::max<u32>(1, params.setting(0) * m_numBars);
    m_angSpeed = static_cast<f32>(static_cast<s16>(params.setting(1)));

    m_fireballs = std::span<ObjectFireball *>(new ObjectFireball *[numFireballs], numFireballs);

    for (size_t i = 0; i < numFireballs; ++i) {
        m_fireballs[i] = new ObjectFireball(params);
        m_fireballs[i]->load();

        f32 scalar = 1.0f + i / m_numBars;
        m_fireballs[i]->setScalar(
                scalar * (100.0f * static_cast<f32>(static_cast<s16>(params.setting(2)))));
        m_fireballs[i]->setb8((360.0f / m_numBars) * (i % m_numBars));
    }

    EGG::Matrix34f mat;
    mat.makeR(m_rot);
    m_cc = mat.base(2);
    m_cc.normalise();
    m_d8 = m_cc.cross(FUN_806B3AC4(F_PI_OVER_2, EGG::Vector3f::ex, m_cc));
    m_d8.normalise();
}

/// @addr{0x807688AC}
ObjectFirebar::~ObjectFirebar() {
    for (auto &fireball : m_fireballs) {
        delete fireball;
    }

    delete m_fireballs.data();
}

/// @addr{0x80767DEC}
void ObjectFirebar::init() {
    m_degAngle = 0.0f;
}

/// @addr{0x80767E04}
void ObjectFirebar::calc() {
    m_degAngle += m_angSpeed / 60.0f;

    if (m_degAngle > 360.0f) {
        m_degAngle -= 360.0f;
    } else if (m_degAngle < 0.0f) {
        m_degAngle += 360.0f;
    }

    for (auto &fireball : m_fireballs) {
        EGG::Vector3f scaledD8 = m_d8 * fireball->scalar();
        fireball->setFlag(1);
        fireball->setPos(
                m_pos + FUN_806B3AC4((m_degAngle + fireball->b8()) * DEG2RAD, m_cc, scaledD8));
    }
}

} // namespace Field
