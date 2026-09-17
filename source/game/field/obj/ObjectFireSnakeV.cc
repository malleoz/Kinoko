#include "ObjectFireSnakeV.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Field {

/// @addr{0x806C2DA4}
/// @brief Updates state lifecycle and children positions once the spawn delay has elapsed
/// @details Evaluates the fire snake's state machine. If @ref m_cycleDuration frames have elapsed
/// since the initial delay, the fire snake will spawn by transitioning to the falling state. If the
/// fire snake is spawned, increments @ref m_age each frame. If the age of the fire snake exceeds
/// `600` frames and the fire snake is at rest, then despawns the fire snake by transitioning to the
/// despawned state. Finally, dispatches to @ref calcChildren() to update the state of the child
/// objects.
void ObjectFireSnakeV::calcSub() {
    constexpr u16 LIFECYCLE_DURATION = 600;

    StateManager::calc();

    u32 frame = System::RaceManager::Instance()->timer() - m_delayFrame;
    if (frame % m_cycleDuration == 0) {
        m_nextStateId = 1;
    }

    if (m_currentStateId >= 1 && m_currentStateId <= 4) {
        ++m_age;
        if (m_age >= LIFECYCLE_DURATION && m_currentStateId == 3) {
            m_nextStateId = 0;
        }
    }

    calcChildren();
}

/// @addr{0x806C30F8}
/// @brief Runs once when the fire snake respawns
/// @details Calls the base class implementation to handle the fire snake's initial falling
/// behavior. Disables the fire snake's collision. Resets the fire snake's age, trajectory position,
/// bounce direction, and fall speed.
void ObjectFireSnakeV::enterFalling() {
    constexpr f32 FALL_DURATION = 140.0f;

    ObjectFireSnake::enterFalling();

    if (getUnit()) {
        unregisterCollision();
    }

    m_age = 0;
    m_trajectoryPos = m_spawnPos;
    m_bounceDir = m_initRot;
    m_initPos = m_spawnPos + m_initRot * m_distFromPipe;
    m_fallSpeed = m_distFromPipe / FALL_DURATION;
}

/// @addr{0x806C31F0}
/// @brief Runs every frame between the fire snake respawning and landing on the ground
/// @details If the fire snake has been falling for more than 5 frames, re-enabled the fire snake's
/// collision. Every frame, updates the fire snake's trajectory position based on its bounce
/// direction and fall speed, and checks for collisions with the floor. If a collision is detected,
/// adjusts the trajectory position and transitions to the high bounce state.
void ObjectFireSnakeV::calcFalling() {
    constexpr f32 INITIAL_Y_VELOCITY = 120.0f;
    constexpr u32 AABB_DELAY_FRAMES = 5;

    if (m_currentFrame > AABB_DELAY_FRAMES && !getUnit()) {
        loadAABB(0.0f);
    }

    m_trajectoryPos.x += m_bounceDir.x * m_fallSpeed;
    m_trajectoryPos.z += m_bounceDir.z * m_fallSpeed;
    m_trajectoryPos.y += INITIAL_Y_VELOCITY - GRAVITY * static_cast<f32>(m_currentFrame);

    if (m_currentFrame > COL_CHECK_DELAY_FRAMES) {
        CollisionInfo colInfo;

        bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, m_trajectoryPos,
                EGG::Vector3f::inf, KCL_TYPE_FLOOR, &colInfo, nullptr, 0);

        if (hasCol) {
            m_trajectoryPos += colInfo.tangentOff;
            m_nextStateId = 2;
        }
    }

    setMatrixTangentTo(EGG::Vector3f::ey, m_bounceDir);
    setPos(m_trajectoryPos);
}

/// @addr{0x806C33C4}
/// @brief Runs every frame during the first bounce
/// @details Updates the fire snake's trajectory position based on its bounce direction and fall
/// speed, with an initial upwards velocity of `90.0f`. If @ref COL_CHECK_DELAY_FRAMES have elapsed,
/// checks for collisions with the floor and transitions to the rest state if a collision is
/// detected.
void ObjectFireSnakeV::calcHighBounce() {
    constexpr f32 INITIAL_Y_VELOCITY = 90.0f;

    m_trajectoryPos.z += m_bounceDir.z * m_fallSpeed;
    m_trajectoryPos.x += m_bounceDir.x * m_fallSpeed;
    m_trajectoryPos.y += INITIAL_Y_VELOCITY - GRAVITY * static_cast<f32>(m_currentFrame);

    if (m_currentFrame > COL_CHECK_DELAY_FRAMES) {
        CollisionInfo colInfo;

        bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, m_trajectoryPos,
                EGG::Vector3f::inf, KCL_TYPE_FLOOR, &colInfo, nullptr, 0);

        if (hasCol) {
            m_trajectoryPos += colInfo.tangentOff;
            m_nextStateId = 3;
        }
    }

    setMatrixTangentTo(EGG::Vector3f::ey, m_bounceDir);
    setPos(m_trajectoryPos);
}

} // namespace Kinoko::Field
