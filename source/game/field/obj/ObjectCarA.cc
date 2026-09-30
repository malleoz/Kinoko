#include "ObjectCarA.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

/// @addr{0x806B7CE0}
/// @copybrief ObjectBase::init()
/// @details Initializes the rail interpolator to the beginning of the rail and sets the car's
/// position to the current position on the rail. Calculates @ref m_cruiseTime based on the rail
/// length, @ref m_finalSpeed, and @ref m_accel. Initializes the car's @ref m_currVel to zero, @ref
/// m_motionState to @ref MotionState::Accelerating, @ref m_currUp to world up, @ref m_currTangent
/// to the rail interpolator's tangent direction, and the car's collision radius to `400.0f`.
/// Finally, transitions the car to the stop state.
void ObjectCarA::init() {
    constexpr f32 RADIUS = 400.0f;

    m_railInterpolator->init(0, 0);

    // Time to reach final velocity
    f32 finalTime = m_finalSpeed / m_accel;
    m_cruiseTime =
            (m_railInterpolator->railLength() - finalTime * m_accel * finalTime) / m_finalSpeed;

    setMatrixFromOrthonormalBasisAndPos(m_railInterpolator->curTangentDir());

    setPos(m_railInterpolator->curPos());
    m_currVel = 0.0f;
    m_motionState = MotionState::Accelerating;
    m_changingDir = false;
    m_currUp = EGG::Vector3f::ey;
    m_currTangent = m_railInterpolator->curTangentDir();
    resize(RADIUS, 0.0f);
    m_nextStateId = 0;
}

/// @addr{0x806B7BC4}
/// @copybrief ObjectBase::calcCollisionTransform()
/// @details Modifies the collision object's transform based on the car's current position, scale,
/// and orientation with an upwards vertical offset of `50.0f` units, possibly to account for the
/// tire height.
void ObjectCarA::calcCollisionTransform() {
    constexpr f32 HEIGHT_OFFSET = 50.0f;

    ObjectCollisionBase *objCol = collision();
    if (!objCol) {
        return;
    }

    calcTransform();

    EGG::Matrix34f mat;
    SetRotTangentHorizontal(mat, transform().base(2), EGG::Vector3f::ey);
    mat.setBase(3, transform().base(3) + HEIGHT_OFFSET * transform().base(1));

    objCol->transform(mat, scale(),
            -m_railInterpolator->curTangentDir() * m_railInterpolator->getCurrSpeed());
}

/// @addr{0x806B7E60}
/// @copybrief ObjectCollidable::onCollision()
/// @param kartObj The kart object that collided with this car
/// @param reactionOnKart The reaction that should be applied to the kart
/// @return @ref Kart::Reaction::Wall if the kart's speed is below 50%, otherwise returns @ref
/// Kart::Reaction::LaunchAwayFlipOnce.
Kart::Reaction ObjectCarA::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) {
    return kartObj->speedRatioCapped() < 0.5f ? Kart::Reaction::Wall : reactionOnKart;
}

/// @addr{0x806B86F0}
/// @brief Runs once per frame when the car is in the accelerating or decelerating state
/// @details If the car is accelerating, @ref m_currVel increases by @ref m_accel until
/// it reaches @ref m_finalSpeed at which point it transitions to the cruising state. If the car is
/// decelerating, @ref m_currVel decreases by the
/// @ref m_accel until it reaches `0.01f`, preventing the car from stopping before reaching the end
/// of the rail. If the car is changing direction, @ref m_currVel is set to zero and the car
/// transitions to the stop state.
void ObjectCarA::calcAccel() {
    if (m_motionState == MotionState::Accelerating) {
        m_currVel += m_accel;

        if (m_currVel > m_finalSpeed) {
            m_currVel = m_finalSpeed;
            m_motionState = MotionState::Cruising;
            m_nextStateId = 2;
        }
    } else if (m_motionState == MotionState::Decelerating) {
        // The cruising time might've had decimals and needed to end early.
        // If we undershot it, keep a velocity of 0.01f so we can reach the end.
        m_currVel = std::max(0.01f, m_currVel - m_accel);

        if (m_changingDir) {
            m_currVel = 0.0f;

            if (m_railInterpolator->isMovementDirectionForward()) {
                m_railInterpolator->init(0.0f, 0);
            }

            m_motionState = MotionState::Cruising;
            m_nextStateId = 0;
        }
    }
}

} // namespace Kinoko::Field
