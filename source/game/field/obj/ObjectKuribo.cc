#include "ObjectKuribo.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Field {

/// @addr{0x806DB40C}
/// @copybrief ObjectBase::init()
/// @details Updates the Goomba's transformation. Caches the Goomba's forward direction to @ref
/// m_forward. Initializes the rail interpolator the start of the rail and zeroes its speed. Sets
/// @ref m_currSpeed, @ref m_animTimer, and @ref m_currFrame to zero. Plays the `walk_l`
/// animation and caches its framecount to @ref m_animDuration. Finally, initializes the Goomba to
/// the walking state.
void ObjectKuribo::init() {
    calcTransform();
    m_forward = transform().base(2);

    m_railInterpolator->init(0.0f, 0);
    m_railInterpolator->setSpeed(0.0f);

    m_currSpeed = 0.0f;
    m_animTimer = 0.0f;
    m_currFrame = 0;

    auto *anmMgr = m_drawMdl->anmMgr();
    anmMgr->playAnim(0.0f, m_animRate, 0);
    m_animDuration = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();
    m_nextStateId = 1;
}

/// @addr{0x806DCDDC}
/// @brief Handles the Goomba's walking animation and movement along the rail
/// @details This class defines during which intervals of the animation the Goomba should actually
/// move, so that the Goomba's movement matches up with its animation. If it's determined that the
/// Goomba should move, its @ref m_currSpeed is computed based on @ref m_accel and clamped to a
/// minimum value of `10.0f`. If the Goomba should not move, it decelerates to a speed of `0.0f`.
/// Sets the rail interpolator's speed and sets the Goomba's position accordingly. If the Goomba has
/// reached the end of the rail, transitions the Goomba to the idle state and zeroes its speed.
/// Finally, performs a collision check with the floor, updates the Goombas rotation, and updates
/// its transform accordingly.
void ObjectKuribo::calcAnim() {
    bool shouldMove;

    if (m_railInterpolator->isMovementDirectionForward()) {
        shouldMove = m_animTimer > 15.0f && m_animTimer < 25.0f;
    } else {
        shouldMove = m_animTimer > 45.0f && m_animTimer < 55.0f;
    }

    if (shouldMove) {
        m_currSpeed = std::min(10.0f, m_currSpeed + m_accel);
    } else {
        m_currSpeed = std::max(0.0f, m_currSpeed - m_accel);
    }

    m_railInterpolator->setSpeed(m_currSpeed);
    const auto &curPos = m_railInterpolator->curPos();
    setPos(EGG::Vector3f(curPos.x, pos().y, curPos.z));

    if (m_railInterpolator->calc() == RailInterpolator::Status::ChangingDirection) {
        m_nextStateId = 0;
        m_currSpeed = 0.0f;
    }

    checkSphereFull();
    calcRot();
    calcMatFromRotAndForward();
}

/// @addr{0x806DCB58}
/// @brief Checks for floor collision beneath the Goomba
/// @details If the Goomba is walking, applies a downward gravitational force of `2.0f` every frame.
/// If the Goomba has collided with the floor, updates the Goomba's position accordingly and caches
/// the colliding floor's normal to @ref m_floorNrm.
void ObjectKuribo::checkSphereFull() {
    constexpr f32 RADIUS = 50.0f;
    constexpr f32 GRAVITY = 2.0f;

    // Apply gravity if we're not changing direction
    if (m_currentStateId != 0) {
        subPos(EGG::Vector3f(0.0f, GRAVITY, 0.0f));
    }

    CollisionInfo colInfo;
    EGG::Vector3f colPos = pos();
    colPos.y += RADIUS;

    bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, colPos, EGG::Vector3f::inf,
            KCL_TYPE_FLOOR, &colInfo, nullptr, 0);

    if (hasCol) {
        addPos(colInfo.tangentOff);

        if (colInfo.floorDist > -std::numeric_limits<f32>::min()) {
            m_floorNrm = colInfo.floorNrm;
        }
    }
}

} // namespace Kinoko::Field
