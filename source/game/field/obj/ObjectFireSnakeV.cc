#include "ObjectFireSnakeV.hh"

#include "game/field/CollisionDirector.hh"

namespace Kinoko::Field {

/// @addr{0x806C2B70}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectFireSnakeV::ObjectFireSnakeV(const System::MapdataGeoObj &params)
    : StateManager(this, STATE_ENTRIES), ObjectFireSnake(params),
      m_cycleDuration(params.setting(1)), m_distFromPipe(static_cast<f32>(params.setting(2))),
      m_fallSpeed(0.0f) {
    m_delayFrame = params.setting(0);
    m_spawnPos = pos();

    calcTransform();

    m_initRot = transform().base(0);
    m_initPos = m_spawnPos + m_initRot * m_distFromPipe;
}

/// @addr{0x806C3548}
/// @brief Default virtual destructor
ObjectFireSnakeV::~ObjectFireSnakeV() = default;

/// @addr{0x806C2DA4}
/// @brief Updates state lifecycle and children positions once the spawn delay has elapsed
void ObjectFireSnakeV::calcSub() {
    StateManager::calc();

    u32 frame = System::RaceManager::Instance()->timer() - m_delayFrame;
    if (frame % m_cycleDuration == 0) {
        m_nextStateId = 1;
    }

    if (m_currentStateId >= 1 && m_currentStateId <= 4) {
        ++m_age;
        if (m_age >= 600 && m_currentStateId == 3) {
            m_nextStateId = 0;
        }
    }

    calcChildren();
}

/// @addr{0x806C30F8}
/// @brief Runs once when the fire snake respawns
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
void ObjectFireSnakeV::calcFalling() {
    constexpr f32 INITIAL_Y_VELOCITY = 120.0f;
    constexpr f32 AABB_DELAY_FRAMES = 5;

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
