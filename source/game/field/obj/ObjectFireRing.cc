#include "ObjectFireRing.hh"

namespace Field {

/// @addr{0x80767FF4}
ObjectFireRing::ObjectFireRing(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), m_phase(0.0f) {
    u32 numFireballs = std::max<u32>(1, params.setting(0));
    m_angSpeed = static_cast<f32>(static_cast<s16>(params.setting(1)));
    m_fireballs = std::span<ObjectFireball *>(new ObjectFireball *[numFireballs], numFireballs);
    f32 scalar = 100.0f * static_cast<f32>(params.setting(3));

    for (size_t i = 0; i < numFireballs; ++i) {
        m_fireballs[i] = new ObjectFireball(params);
        m_fireballs[i]->load();
        m_fireballs[i]->setScalar(scalar);
        m_fireballs[i]->setb8(static_cast<f32>(i) * (360.0f / numFireballs));
    }

    EGG::Matrix34f mat;
    mat.makeR(m_rot);
    m_c8 = mat.base(2);
    m_c8.normalise();
    m_d4 = m_c8.cross(FUN_806B3AC4(F_PI_OVER_2, EGG::Vector3f::ex, m_c8));
    m_d4.normalise();
    m_radiusScale = 0.1f * static_cast<f32>(params.setting(2));
}

/// @addr{0x8076892C}
ObjectFireRing::~ObjectFireRing() {
    for (auto &fireball : m_fireballs) {
        delete fireball;
    }

    delete m_fireballs.data();
}

/// @addr{0x807683F0}
void ObjectFireRing::init() {
    m_degAngle = 0.0f;
}

/// @addr{0x80768408}
void ObjectFireRing::calc() {
    m_phase += 1.0f;
    m_degAngle += m_angSpeed / 60.0f;

    if (m_degAngle > 360.0f) {
        m_degAngle -= 360.0f;
    } else if (m_degAngle < 0.0f) {
        m_degAngle += 360.0f;
    }

    f32 radius = m_radiusScale * EGG::Mathf::sin(m_phase * DEG2RAD);

    for (auto &fireball : m_fireballs) {
        EGG::Vector3f scaledD4 = m_d4 * fireball->scalar() * (1.0f + radius);
        fireball->setFlag(1);
        fireball->setPos(
                m_pos + FUN_806B3AC4((m_degAngle + fireball->b8()) * DEG2RAD, m_c8, scaledD4));
    }
}

} // namespace Field
