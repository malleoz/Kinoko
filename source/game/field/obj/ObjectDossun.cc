#include "ObjectDossun.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @addr{0x8075EEA8}
/// @copybrief ObjectBase::init()
/// @details Initializes the Thwomp's animation state to @ref AnmState::Still and calls @ref
/// initState(). Caches the Thwomp's initial vertical position to @ref m_initialPosY and clears @ref
/// m_vel. Mainly, this function determines the total duration of the Thwomp's stomp cycle based off
/// of how long it takes the Thwomp to stomp down onto the floor beneath it.
/// @note Since @ref m_fullDuration computes the expected duration of the stomp animation, Thwomps
/// will end stomping at this duration, even if the floor beneath it has been lowered. This may
/// result in Thwomp stomps ending mid-air on some custom tracks.
void ObjectDossun::init() {
    constexpr f32 BEFORE_FALL_VEL = 30.0f;

    m_anmState = AnmState::Still;

    initState();

    if (m_railInterpolator) {
        m_railInterpolator->init(0.0f, 0);
    }

    m_initialPosY = pos().y;
    m_vel = 0.0f;

    for (u32 i = 0; i < BEFORE_FALL_DURATION; ++i) {
        setPos(EGG::Vector3f(pos().x, BEFORE_FALL_VEL + pos().y, pos().z));
    }

    bool hasCol = false;
    auto *colDir = CollisionDirector::Instance();
    CollisionInfo info;
    u32 frameCount = 0;

    do {
        ++frameCount;
        m_vel -= STOMP_ACCEL;
        setPos(EGG::Vector3f(pos().x, m_vel + pos().y, pos().z));

        info.reset();
        EGG::Vector3f colPos = pos() + STOMP_POS_OFFSET;

        hasCol = colDir->checkSphereFull(STOMP_RADIUS, colPos, EGG::Vector3f::inf, KCL_TYPE_FLOOR,
                &info, nullptr, 0);
    } while (!hasCol);

    addPos(info.tangentOff);
    m_vel = 0.0f;

    f32 fallDuration = frameCount;

    frameCount = 0;
    f32 riseDuration;

    while (true) {
        f32 posY = RISING_VEL + pos().y;
        ++frameCount;

        if (posY >= m_initialPosY) {
            setPos(EGG::Vector3f(pos().x, m_initialPosY, pos().z));
            riseDuration = frameCount;
            break;
        } else {
            setPos(EGG::Vector3f(pos().x, posY, pos().z));
        }
    }

    m_fullDuration = fallDuration + GROUND_DURATION + riseDuration + BEFORE_FALL_DURATION;
    m_touchingGround = false;
}

/// @addr{0x8075FF98}
/// @copybrief ObjectCollidable::onCollision()
/// @param kartObj The kart object that collided with this object
/// @param reactionOnKart The reaction that should be applied to the kart upon collision
/// @param hitDepth The depth of the collision between the kart and the object
/// @return @ref Kart::Rection::LongCrushLoseItem if the player should be crushed, otherwise @ref
/// Kart::Reaction::Wall.
/// @details Squishes the player if they are within a certain distance of a falling Thwomp. A squish
/// can still technically occur when the Thwomp is touching the ground, but this is not possible in
/// practice because the player will be pushed away by the Thwomp's collision before it gets within
/// the squish radius.
Kart::Reaction ObjectDossun::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) {
    constexpr f32 SQUISH_DISTANCE = 375.0f;

    EGG::Vector3f xzDist = kartObj->pos() - pos();
    xzDist.y = 0.0f;

    if (xzDist.length() < SQUISH_DISTANCE * scale().x &&
            (m_anmState == AnmState::Falling || m_touchingGround)) {
        const auto &hitTable = ObjectDirector::Instance()->hitTableKart();
        return hitTable.reaction(hitTable.slot(ObjectId::DossuncSoko));
    }

    return reactionOnKart;
}

/// @addr{0x8075F254}
/// @brief Updates the Thwomp's animation state related to stomping
/// @details Calls the appropriate calculation function based on the current animation state. If the
/// stomp duration reaches zero, transitions the Thwomp to the still state.
void ObjectDossun::calcStomp() {
    switch (m_anmState) {
    case AnmState::BeforeFall:
        calcBeforeFall();
        break;
    case AnmState::Falling:
        calcFalling();
        break;
    case AnmState::Grounded:
        calcGrounded();
        break;
    case AnmState::Rising:
        calcRising();
        break;
    default:
        break;
    }

    if (m_stompDuration-- == 0) {
        startStill();
    }
}

/// @addr{0x8075F76C}
/// @brief Checks for collision with the floor while the Thwomp is falling
/// @details Uses a sphere radius of size @ref STOMP_RADIUS to check for collisions with the floor.
/// If a collision occurs, sets @ref m_vel to zero, applies the collision offset to the Thwomp's
/// position to prevent it from clipping into the floor, calls @ref startGrounded(), and sets @ref
/// m_touchingGround to true.
void ObjectDossun::checkFloorCollision() {
    CollisionInfo info;
    EGG::Vector3f colPos = pos() + STOMP_POS_OFFSET;

    if (!CollisionDirector::Instance()->checkSphereFull(STOMP_RADIUS, colPos, EGG::Vector3f::inf,
                KCL_TYPE_FLOOR, &info, nullptr, 0)) {
        return;
    }

    m_vel = 0.0f;
    addPos(info.tangentOff);

    startGrounded();
    m_touchingGround = true;
}

} // namespace Kinoko::Field
