#include "ObjectRock.hh"

#include "game/field/CollisionDirector.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

/// @addr{0x8076F384}
/// @copybrief ObjectBase::init()
/// @details Initializes the rail interpolator to the start of the rail with a speed of @ref
/// m_railSpeed. Updates the rail interpolator once. Sets the rock's @ref m_state to @ref
/// State::Tangible and initializes its position and @ref m_startYPos to the rail interpolator's
/// current height. Sets the rock's transformation matrix based on the rail's tangent direction.
/// Initializes @ref m_angRad to zero, @ref m_angSpd to @ref INITIAL_ANGULAR_SPEED, and sets @ref
/// m_cooldownTimer based on param setting 1. Finally, sets the Y-component of @ref m_vel to @ref
/// m_bounceFactor.
void ObjectRock::init() {
    m_railInterpolator->init(0.0f, 0);
    m_railInterpolator->setSpeed(m_railSpeed);
    m_railInterpolator->calc();

    m_state = State::Tangible;
    f32 posY = m_startYPos = m_railInterpolator->curPos().y;
    setPos(EGG::Vector3f(pos().x, posY, pos().z));

    EGG::Vector3f curTanDirNorm = m_railInterpolator->curTangentDir();
    curTanDirNorm.normalise();
    setMatrixTangentTo(EGG::Vector3f::ey, curTanDirNorm);
    calcTransform();
    m_angRad = 0.0f;
    m_angSpd = INITIAL_ANGULAR_SPEED;
    m_cooldownTimer = m_mapObj->setting(0);
    m_vel.x = 0.0f;
    m_vel.y = m_bounceFactor;
    m_vel.z = 0.0f;
    calcTransform();
}

/// @addr{0x8076F590}
/// @copybrief ObjectBase::calc()
/// @details Calls the appropriate calculation function (either @ref calcTangible() or @ref
/// calcIntangible()) based on the rock's current state. Decrements @ref m_cooldownTimer. Sets the
/// rock's velocity based on the rail's tangent direction and current speed.
void ObjectRock::calc() {
    switch (m_state) {
    case State::Tangible:
        calcTangible();
        break;
    case State::Intangible:
        calcIntangible();
        break;
    }

    --m_cooldownTimer;
    EGG::Vector3f scaledTang =
            m_railInterpolator->curTangentDir() * m_railInterpolator->getCurrSpeed();
    m_vel.x = scaledTang.x;
    m_vel.z = scaledTang.z;
}

/// @addr{0x8076F768}
/// @brief Runs every frame that the rock is collidable
/// @details Updates the rock's position along the rail. If the rock has reached the end of the
/// rail, it breaks via @ref breakRock() and becomes intangible. Applies a downward gravitational
/// force of `2.0f` and updates the rock's position accordingly. Checks to see if the rock has
/// collided with the floor. Finally, updates the rock's rotation based on its angular velocity.
void ObjectRock::calcTangible() {
    constexpr f32 GRAVITY = 2.0f;

    auto railStatus = m_railInterpolator->calc();
    if (railStatus == RailInterpolator::Status::ChangingDirection) {
        breakRock();
    }

    const auto &railPos = m_railInterpolator->curPos();
    setPos(EGG::Vector3f(railPos.x, pos().y, railPos.z));
    m_vel.y -= GRAVITY;

    addPos(m_vel);

    checkSphereFull();
    calcTangibleSub();
}

/// @addr{0x8076F91C}
/// @brief Updates the rock's rotation based on its angular velocity
/// @details Increments the rock's @ref m_angRad based on @ref m_angSpd and updates its rotation
/// matrix accordingly.
void ObjectRock::calcTangibleSub() {
    EGG::Vector3f tangDir = m_railInterpolator->curTangentDir();
    tangDir.y = 0.0f;
    tangDir.normalise();
    EGG::Matrix34f m = OrthonormalBasis(tangDir);
    m_angRad += m_angSpd * DEG2RAD;

    EGG::Matrix34f mat(EGG::Matrix34f::ident);
    EGG::Vector3f vRot(m_angRad, 0.0f, 0.0f);
    mat.makeR(vRot);
    mat = m.multiplyTo(mat);
    mat.setBase(3, pos());
    setTransform(mat);
}

/// @addr{0x8076FA60}
/// @brief Checks for collisions with the floor and applies a bounce effect if a collision occurs
/// @details Offsets the rock's position downward so that the collision check occurs with the
/// bottommost part of the rock. If a floor collision occurs, bounces the rock upwards with a 70%
/// dampener applied to its velocity, increases the rock's @ref m_angSpd (with a minimum value based
/// on the rail's speed), and updates the rock's position accordingly,
void ObjectRock::checkSphereFull() {
    constexpr f32 RADIUS = 50.0f;

    CollisionInfo info;

    EGG::Vector3f offset(0.0f, -(scale().x * ROCK_RADIUS - RADIUS), 0.0f);
    EGG::Vector3f colPos = pos() + offset;

    if (CollisionDirector::Instance()->checkSphereFull(RADIUS, colPos, EGG::Vector3f::inf,
                KCL_TYPE_FLOOR, &info, nullptr, 0)) {
        m_vel.y *= -0.3f;
        m_angSpd = std::max(360.0f * m_railSpeed / ((2.0f * ROCK_RADIUS) * scale().x * F_PI),
                m_angSpd + 1.0f);
        addPos(info.tangentOff);
    }
}

/// @addr{0x80770068}
/// @copybrief ObjectCollidable::onCollision()
/// @param reactionOnKart The reaction that should be applied to the kart upon collision
/// @return @ref Kart::Reaction::None if the rock is small, otherwise @ref Kart::Reaction::Sideways.
/// @details Collision with small rocks breaks the rock without affecting the player.
Kart::Reaction ObjectRock::onCollision(Kart::KartObject * /*kartObj*/,
        Kart::Reaction reactionOnKart, Kart::Reaction /*reactionOnObj*/,
        EGG::Vector3f & /*hitDepth*/) {
    if (scale().x < 1.0f) {
        breakRock();
        return Kart::Reaction::None;
    }
    return reactionOnKart;
}

} // namespace Kinoko::Field
