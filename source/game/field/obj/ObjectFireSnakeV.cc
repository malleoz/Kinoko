#include "ObjectFireSnakeV.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @addr{0x806C2B70}
ObjectFireSnakeV::ObjectFireSnakeV(const System::MapdataGeoObj &params)
    : ObjectFireSnake(params), StateManager<ObjectFireSnakeV>(this),
      m_cycleDuration(params.setting(1)), m_distFromPipe(static_cast<f32>(params.setting(2))) {
    m_delayFrame = params.setting(0);
    m_sunPos = m_pos;

    calcTransform();

    m_dirFromPipe = m_transform.base(0);
    m_initialPos = m_sunPos + m_dirFromPipe * m_distFromPipe;
}

/// @addr{0x806C3548}
ObjectFireSnakeV::~ObjectFireSnakeV() = default;

/// @addr{0x806C2CBC}
void ObjectFireSnakeV::init() {
    StateManager<ObjectFireSnakeV>::m_nextStateId = 1;
    ObjectFireSnake::enterDespawned();

    m_nextPos = m_sunPos;
    m_tangent = m_dirFromPipe;
}

/// @addr{0x806C2D54}
void ObjectFireSnakeV::calc() {
    if (System::RaceManager::Instance()->timer() < m_delayFrame) {
        return;
    }

    FUN_806C2DA4();
}

/// @addr{0x806C30F8}
void ObjectFireSnakeV::enterFalling() {
    constexpr f32 FALL_DURATION = 140.0f;

    ObjectFireSnake::enterFalling();

    if (getUnit()) {
        unregisterCollision();
    }

    m_age = 0;
    m_nextPos = m_sunPos;
    m_tangent = m_dirFromPipe;
    m_initialPos = m_sunPos + m_dirFromPipe * m_distFromPipe;
    m_fallSpeed = m_distFromPipe / FALL_DURATION;
}

/// @addr{0x806C31F0}
void ObjectFireSnakeV::calcFalling() {
    constexpr f32 INITIAL_Y_VELOCITY = 120.0f;

    if (StateManager<ObjectFireSnakeV>::m_currentFrame > 5 && !getUnit()) {
        loadAABB(0.0f);
    }

    m_nextPos.x += m_tangent.x * m_fallSpeed;
    m_nextPos.z += m_tangent.z * m_fallSpeed;
    m_nextPos.y += INITIAL_Y_VELOCITY -
            GRAVITY * static_cast<f32>(StateManager<ObjectFireSnakeV>::m_currentFrame);

    if (StateManager<ObjectFireSnakeV>::m_currentFrame > 10) {
        CollisionInfo colInfo;

        bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, m_nextPos,
                EGG::Vector3f::inf, KCL_TYPE_FLOOR, &colInfo, nullptr, 0);

        if (hasCol) {
            m_nextPos += colInfo.tangentOff;
            StateManager<ObjectFireSnakeV>::m_nextStateId = 2;
        }
    }

    setMatrixTangentTo(EGG::Vector3f::ey, m_tangent);
    m_flags.setBit(eFlags::Position);
    m_pos = m_nextPos;
}

/// @addr{0x806C33C4}
void ObjectFireSnakeV::calcHighBounce() {
    constexpr f32 INITIAL_Y_VELOCITY = 90.0f;

    m_nextPos.z += m_tangent.z * m_fallSpeed;
    m_nextPos.x += m_tangent.x * m_fallSpeed;
    m_nextPos.y += INITIAL_Y_VELOCITY -
            GRAVITY * static_cast<f32>(StateManager<ObjectFireSnakeV>::m_currentFrame);

    if (StateManager<ObjectFireSnakeV>::m_currentFrame > 10) {
        CollisionInfo colInfo;

        bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, m_nextPos,
                EGG::Vector3f::inf, KCL_TYPE_FLOOR, &colInfo, nullptr, 0);

        if (hasCol) {
            m_nextPos += colInfo.tangentOff;
            StateManager<ObjectFireSnakeV>::m_nextStateId = 3;
        }
    }

    setMatrixTangentTo(EGG::Vector3f::ey, m_tangent);
    m_flags.setBit(eFlags::Position);
    m_pos = m_nextPos;
}

/// @addr{0x806C2DA4}
void ObjectFireSnakeV::FUN_806C2DA4() {
    StateManager<ObjectFireSnakeV>::calc();

    u32 frame = System::RaceManager::Instance()->timer() - m_delayFrame;
    if (frame % m_cycleDuration == 0) {
        StateManager<ObjectFireSnakeV>::m_nextStateId = 1;
    }

    if (StateManager<ObjectFireSnakeV>::m_currentStateId == 1 ||
            StateManager<ObjectFireSnakeV>::m_currentStateId == 2 ||
            StateManager<ObjectFireSnakeV>::m_currentStateId == 3 ||
            StateManager<ObjectFireSnakeV>::m_currentStateId == 4) {
        ++m_age;
        if (m_age > 600 && StateManager<ObjectFireSnakeV>::m_currentStateId == 3) {
            StateManager<ObjectFireSnakeV>::m_nextStateId = 0;
        }
    }

    calcChildren();
}

} // namespace Field
