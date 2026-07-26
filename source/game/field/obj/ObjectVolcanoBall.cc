#include "ObjectVolcanoBall.hh"

namespace Kinoko::Field {

/// @addr{0x806E2904}
ObjectVolcanoBall::ObjectVolcanoBall(f32 accel, f32 finalVel, f32 endPosY,
        const System::MapdataGeoObj &params, const EGG::Vector3f &vel)
    : ObjectCollidable(params), StateManager(this, STATE_ENTRIES),
      m_burnDuration(params.setting(3)), m_accel(accel), m_finalVelSq(finalVel), m_endPosY(endPosY),
      m_sqVelXZ(vel.x * vel.x + vel.z * vel.z) {}

/// @addr{0x806E2BE0}
ObjectVolcanoBall::~ObjectVolcanoBall() = default;

/// @addr{0x806E3034}
/// @brief Calculates the ball's velocity and updates its position along the rail
/// @details Derives the ball's velocity based on the kinematic equation:
/// \f[
/// v^2 = v_f^2 - 2a(y - y_{end})
/// \f]
/// where \f$v_f\f$ is the final velocity at impact, \f$a\f$ is the acceleration,
/// \f$y\f$ is the current height, and \f$y_{end}\f$ is the end height.
void ObjectVolcanoBall::calcFalling() {
    f32 sqVel = std::max(0.01f, m_finalVelSq - 2.0f * m_accel * (pos().y - m_endPosY));
    m_railInterpolator->setCurrVel(EGG::Mathf::sqrt(m_sqVelXZ + sqVel));

    if (m_railInterpolator->calc() == RailInterpolator::Status::ChangingDirection) {
        m_nextStateId = 2;
    } else {
        EGG::Vector3f tangent = m_railInterpolator->curTangentDir();
        if (EGG::Mathf::abs(tangent.y) > 0.1f) {
            tangent.y = 0.01f;
        }

        tangent.normalise2();
        setMatrixFromOrthonormalBasisAndPos(tangent);

        setPos(m_railInterpolator->curPos());
    }
}

} // namespace Kinoko::Field
