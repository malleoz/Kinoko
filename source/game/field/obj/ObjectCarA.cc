#include "ObjectCarA.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

/// @addr{0x806B7710}
/// @brief Constructor
/// @param params The parameters used to initialize the object
/// @details Computes the cruising speed, acceleration, and stop duration based on the provided
/// parameter settings.
ObjectCarA::ObjectCarA(const System::MapdataGeoObj &params)
    : ObjectCollidable(params),
      StateManager(this, STATE_ENTRIES),
      m_finalSpeed(static_cast<f32>(params.setting(0))),
      m_accel(static_cast<f32>(params.setting(1)) / 10.0f),
      m_stopDuration(static_cast<u32>(params.setting(2))) {}

/// @addr{0x806B7CE0}
/// @copybrief ObjectBase::init()
/// @details Initializes the rail interpolator and sets the car's position to the current position
/// on the rail. Calculates @ref m_cruiseTime based on the rail length. Initializes the car's
/// velocity, orientation vectors, and collision radius.
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
/// and orientation with a height offset applied, possibly to account for the tire height.
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
            -m_railInterpolator->curTangentDir() * m_railInterpolator->getCurrVel());
}

/// @addr{0x806B7E60}
/// @copybrief ObjectCollidable::onCollision()
/// @param kartObj The kart object that collided with this car
/// @param reactionOnKart The reaction that should be applied to the kart
/// @return The reaction that should be applied to the kart
/// @details Cars act like walls when the kart is at 50% speed or lower. Otherwise, returns @ref
/// Action::LaunchAwayFlipOnce.
Kart::Reaction ObjectCarA::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) {
    return kartObj->speedRatioCapped() < 0.5f ? Kart::Reaction::Wall : reactionOnKart;
}

/// @addr{0x806B86F0}
/// @brief Runs once per frame when the car is in the accelerating or decelerating state
/// @details If the car is accelerating, its velocity is increased by the acceleration value until
/// it reaches the final speed. If the car is decelerating, its velocity is decreased by the
/// acceleration value until it reaches zero, with special handling for changing direction.
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
