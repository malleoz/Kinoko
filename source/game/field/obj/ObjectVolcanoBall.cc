#include "ObjectVolcanoBall.hh"

namespace Field {

/// @addr{0x806E2904}
ObjectVolcanoBall::ObjectVolcanoBall(f32 param1, f32 param2, f32 param3,
        const System::MapdataGeoObj &params, const EGG::Vector3f &vec)
    : ObjectCollidable(params), StateManager(this) {
    m_initialVel = static_cast<f32>(static_cast<s16>(params.setting(0)));
    m_state2Duration = params.setting(3);
    m_e4 = param1;
    m_e8 = param2;
    m_ec = param3;
    m_f0 = vec.x * vec.x + vec.z * vec.z;
}

/// @addr{0x806E2BE0}
ObjectVolcanoBall::~ObjectVolcanoBall() = default;

/// @addr{0x806E2C4C}
void ObjectVolcanoBall::init() {
    m_railInterpolator->init(0.0f, 0);
    m_vel = m_initialVel;
    m_pos = m_railInterpolator->curPos();
    m_flags.setBit(eFlags::Position);
}

/// @addr{0x806E2E08}
void ObjectVolcanoBall::calc() {
    StateManager::calc();
}

/// @addr{0x806E3034}
void ObjectVolcanoBall::calcState1() {
    f32 fVar6 = std::max(0.01f, m_e8 - 2.0f * m_e4 * (m_pos.y - m_ec));
    m_vel = EGG::Mathf::sqrt(m_f0 + fVar6);

    m_railInterpolator->setCurrVel(m_vel);
    if (m_railInterpolator->calc() == RailInterpolator::Status::ChangingDirection) {
        m_nextStateId = 2;
    } else {
        EGG::Vector3f tangent = m_railInterpolator->curTangentDir();
        if (EGG::Mathf::abs(tangent.y) > 0.1f) {
            tangent.y = 0.01f;
        }

        tangent.normalise2();
        setMatrixFromOrthonormalBasisAndPos(tangent);

        m_pos = m_railInterpolator->curPos();
        m_flags.setBit(eFlags::Position);
    }
}

/// @addr{0x806E3324}
void ObjectVolcanoBall::calcState2() {
    if (m_currentFrame >= m_state2Duration) {
        m_nextStateId = 0;
    }
}

} // namespace Field
