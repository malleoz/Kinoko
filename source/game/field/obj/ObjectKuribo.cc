#include "ObjectKuribo.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/CourseColMgr.hh"

#include <cmath>

namespace Field {

/// @addr{0x806DB184}
ObjectKuribo::ObjectKuribo(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
    m_b8 = -1;
    m_b4 = 0;
    m_animStep = static_cast<f32>(m_globalObj.setting(2)) / 100.0f;
    m_speedStep = static_cast<f32>(m_globalObj.setting(1));
}

/// @addr{0x806DB3A0}
ObjectKuribo::~ObjectKuribo() = default;

/// @addr{0x806DB40C}
void ObjectKuribo::init() {
    m_origin = m_transform.base(2);

    m_railInterpolator->init(0.0f, 0);
    m_railInterpolator->setCurrVel(0.0f);

    m_currSpeed = 0.0f;
    m_animTimer = 0.0f;
    m_frameCount = 0;
    m_pos = m_origin;

    // This is set by querying into the DrawMdl... Let's hope this is constant!
    m_maxAnimTimer = 59;
    m_b8 = 1;
}

/// @addr{0x806DB5B0}
void ObjectKuribo::calc() {
    m_animTimer = std::fmod(static_cast<f32>(m_frameCount) * m_animStep, m_maxAnimTimer);

    if (m_b8 >= 0) {
        m_b4 = m_b8;
        m_b8 = -1;
    }

    FUN_806DC3F8();

    ++m_frameCount;
}

/// @addr{0x806DD2C8}
u32 ObjectKuribo::loadFlags() const {
    return 3;
}

/// @addr{0x806DC3F8}
void ObjectKuribo::FUN_806DC3F8() {
    calcAnim();
}

/// @addr{0x806DCDDC}
void ObjectKuribo::calcAnim() {
    bool shouldMove;

    if (m_railInterpolator->isMovementDirectionForward()) {
        shouldMove = 45.0f < m_animTimer && m_animTimer < 55.0f;
    } else {
        shouldMove = 15.0f < m_animTimer && m_animTimer < 25.0f;
    }

    if (shouldMove) {
        m_currSpeed = std::min(10.0f, m_currSpeed + m_speedStep);
    } else {
        m_currSpeed = std::max(0.0f, m_currSpeed - m_speedStep);
    }

    m_railInterpolator->setCurrVel(m_currSpeed);
    m_pos = m_railInterpolator->curPos();

    if (m_railInterpolator->calc() == 2) {
        m_b8 = 0;
        m_currSpeed = 0.0f;
    }

    checkSphereFull();
    m_rot = FUN_806dcd48(0.1f, m_rot, m_floorNrm);

    if (std::numeric_limits<f32>::epsilon() < m_rot.dot()) {
        m_rot.normalise2();
    } else {
        m_rot = EGG::Vector3f::ey;
    }

    setMatrixTangentTo(m_rot, m_origin);
}

/// @addr{0x806DCB58}
void ObjectKuribo::checkSphereFull() {
    constexpr f32 RADIUS = 50.0f;

    if (m_b4 != 0) {
        m_flags |= 1;
        m_pos.y -= 2.0f;
    }

    CourseColMgr::CollisionInfo colInfo;
    EGG::Vector3f pos = m_pos;
    pos.y += RADIUS;

    bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, pos, EGG::Vector3f::inf,
            KCL_TYPE_FLOOR, &colInfo, nullptr, 0);

    if (hasCol) {
        m_pos += colInfo.tangentOff;
        m_flags |= 1;

        if (-std::numeric_limits<f32>::min() < colInfo.floorDist) {
            m_floorNrm = colInfo.floorNrm;
        }
    }
}

/// @addr{0x806DCD48}
EGG::Vector3f ObjectKuribo::FUN_806dcd48(f32 scale, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1) const {
    return v0 + (v1 - v0) * scale;
}

} // namespace Field
