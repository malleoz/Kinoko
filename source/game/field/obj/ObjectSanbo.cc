#include "ObjectSanbo.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/KColData.hh"

namespace Field {

/// @addr{0x80779F3C}
ObjectSanbo::ObjectSanbo(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), m_yVelocity(0.0f) {}

/// @addr{0x8077A1A8}
ObjectSanbo::~ObjectSanbo() = default;

/// @addr{0x8077A1E8}
void ObjectSanbo::init() {
    m_up = EGG::Vector3f::ey;
    EGG::Matrix34f rot;
    rot.makeR(m_rot);
    m_tangent = rot.base(2);
    m_tangent.normalise();
    m_standstill = false;
    m_railInterpolator->init(0.0f, 0);
}

/// @addr{0x8077A36C}
void ObjectSanbo::calc() {
    calcMove();
}

/// @addr{0x8077A5F8}
void ObjectSanbo::calcMove() {
    constexpr f32 GRAVITY = 2.0f;

    if (m_standstill) {
        if (--m_movingDelay == 0) {
            m_standstill = false;
        }

        return;
    }

    auto railStatus = m_railInterpolator->calc();
    if (railStatus == RailInterpolator::Status::ChangingDirection) {
        m_standstill = true;
        m_movingDelay = m_railInterpolator->curPoint().setting[0];
    }

    m_flags |= 1;
    m_yVelocity -= GRAVITY;

    const EGG::Vector3f &railPos = m_railInterpolator->curPos();
    m_pos.x = railPos.x;
    m_pos.z = railPos.z;
    m_pos.y = m_yVelocity + m_pos.y;

    checkSphere();
}

/// @addr{0x8077A8B8}
void ObjectSanbo::checkSphere() {
    constexpr f32 RADIUS = 10.0f;

    CollisionInfo colInfo;
    colInfo.bbox.setZero();

    EGG::Vector3f local_84 = m_up;
    EGG::Vector3f pos = m_pos + EGG::Vector3f(0.0f, RADIUS, 0.0f);

    if (CollisionDirector::Instance()->checkSphereFull(RADIUS, pos, EGG::Vector3f::inf,
                KCL_TYPE_FLOOR, &colInfo, nullptr, 0)) {
        m_yVelocity = 0.0f;
        m_flags |= 1;
        m_pos += colInfo.tangentOff;
        local_84 = colInfo.floorNrm;
    }

    EGG::Vector3f upNrm = m_up;
    upNrm.normalise();
    local_84.normalise();

    m_up += (local_84 - upNrm).multInv(60.0f);
    m_up.normalise();

    setMatrixTangentTo(m_up, m_tangent);
}

} // namespace Field
